//
// Created by kaiser on 2019/11/2.
//

#include "code_gen.h"

#include <cassert>
#include <vector>

#include <llvm/IR/Attributes.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetOptions.h>

#include "calc.h"
#include "error.h"
#include "llvm_common.h"
#include "util.h"

namespace kcc {

/*
 * BreakContinue
 */
CodeGen::BreakContinue::BreakContinue(llvm::BasicBlock* break_block,
                                      llvm::BasicBlock* continue_block)
    : break_block(break_block), continue_block(continue_block) {}

/*
 * CodeGen
 */
void CodeGen::GenCode(const TranslationUnit* root) {
  if (Debug) {
    debug_info_ = std::make_unique<DebugInfo>();
  }

  root->Accept(*this);

  if (debug_info_) {
    debug_info_->Finalize();
  }

  if (llvm::verifyModule(*Module, &llvm::errs())) {
#ifdef DEV
    Warning("module '{}' is broken", Module->getName().str());
#else
    Error("module '{}' is broken", Module->getName().str());
#endif
  }

  assert(std::empty(break_continue_stack_));
  assert(std::empty(labels_));
  assert(!is_bit_field_);
  assert(!bit_field_);
  assert(!is_volatile_);
}

llvm::BasicBlock* CodeGen::CreateBasicBlock(const std::string& name,
                                            llvm::Function* parent) {
  (void)name;
#ifdef NDEBUG
  return llvm::BasicBlock::Create(Context, "", parent);
#else
  return llvm::BasicBlock::Create(Context, name, parent);
#endif
}

void CodeGen::EmitBlock(llvm::BasicBlock* bb, bool is_finished) {
  EmitBranch(bb);

  if (is_finished && bb->use_empty()) {
    delete bb;
    return;
  }

  func_->getBasicBlockList().push_back(bb);
  Builder.SetInsertPoint(bb);
}

void CodeGen::EmitBranch(llvm::BasicBlock* target) {
  auto curr{Builder.GetInsertBlock()};

  if (curr && !curr->getTerminator()) {
    Builder.CreateBr(target);
  }

  Builder.ClearInsertionPoint();
}

bool CodeGen::HaveInsertPoint() const {
  return Builder.GetInsertBlock() != nullptr;
}

void CodeGen::EnsureInsertPoint() {
  if (!HaveInsertPoint()) {
    EmitBlock(CreateBasicBlock());
  }
}

llvm::Value* CodeGen::EvaluateExprAsBool(const Expr* expr) {
  assert(expr != nullptr);
  // Called C++ object pointer is null
  expr->Accept(*this);
  return CastToBool(result_);
}

void CodeGen::EmitBranchOnBoolExpr(const Expr* expr,
                                   llvm::BasicBlock* true_block,
                                   llvm::BasicBlock* false_block) {
  if (auto cond_binary{dynamic_cast<const BinaryOpExpr*>(expr)}) {
    if (cond_binary->GetOp() == Tag::kAmpAmp) {
      // (a && b) && c
      auto lhs_true_block{CreateBasicBlock("logic.and.lhs.true")};
      EmitBranchOnBoolExpr(cond_binary->GetLHS(), lhs_true_block, false_block);
      EmitBlock(lhs_true_block);
      EmitBranchOnBoolExpr(cond_binary->GetRHS(), true_block, false_block);
      return;
    } else if (cond_binary->GetOp() == Tag::kPipePipe) {
      // (a || b) || c
      auto lhs_false_block{CreateBasicBlock("logic.or.lhs.false")};
      EmitBranchOnBoolExpr(cond_binary->GetLHS(), true_block, lhs_false_block);
      EmitBlock(lhs_false_block);
      EmitBranchOnBoolExpr(cond_binary->GetRHS(), true_block, false_block);
      return;
    }
  } else if (auto cond_unary{dynamic_cast<const UnaryOpExpr*>(expr)}) {
    if (cond_unary->GetOp() == Tag::kExclaim) {
      EmitBranchOnBoolExpr(cond_unary->GetExpr(), false_block, true_block);
      return;
    }
  } else if (auto cond_op{dynamic_cast<const ConditionOpExpr*>(expr)}) {
    // (x ? a : b) ? c: d
    auto lhs_block{CreateBasicBlock("cond.true")};
    auto rhs_block{CreateBasicBlock("cond.false")};
    EmitBranchOnBoolExpr(cond_op->GetCond(), lhs_block, rhs_block);
    EmitBlock(lhs_block);
    EmitBranchOnBoolExpr(cond_op->GetLHS(), true_block, false_block);
    EmitBlock(rhs_block);
    EmitBranchOnBoolExpr(cond_op->GetRHS(), true_block, false_block);
    return;
  }

  Builder.CreateCondBr(EvaluateExprAsBool(expr), true_block, false_block);
}

void CodeGen::SimplifyForwardingBlocks(llvm::BasicBlock* bb) {
  auto bi{llvm::dyn_cast<llvm::BranchInst>(bb->getTerminator())};

  if (!bi || !bi->isUnconditional()) {
    return;
  }

  // getSuccessor 获取后继
  bb->replaceAllUsesWith(bi->getSuccessor(0));
  bi->eraseFromParent();
  bb->eraseFromParent();
}

void CodeGen::EmitStmt(const Stmt* stmt) {
  assert(stmt != nullptr);

  // 检查是否可以 emit 而不必考虑 insert point
  if (EmitSimpleStmt(stmt)) {
    return;
  }

  // 检查是否在生成不可访问的代码
  if (!HaveInsertPoint()) {
    // 如果语句不包含 label 则可以不生成它,
    // 这样是安全的, 因为 (1) 改代码不可访问
    // (2) 已经处理了声明
    if (!ContainsLabel(stmt)) {
      assert(stmt->Kind() != AstNodeType::kDeclaration);
      return;
    } else {
      EnsureInsertPoint();
    }
  }

  stmt->Accept(*this);
}

bool CodeGen::EmitSimpleStmt(const Stmt* stmt) {
  switch (stmt->Kind()) {
    case AstNodeType::kLabelStmt:
    case AstNodeType::kCaseStmt:
    case AstNodeType::kDefaultStmt:
    case AstNodeType::kCompoundStmt:
    case AstNodeType::kGotoStmt:
    case AstNodeType::kContinueStmt:
    case AstNodeType::kBreakStmt:
    case AstNodeType::kDeclaration:
      stmt->Accept(*this);
      return true;
    default:
      return false;
  }
}

bool CodeGen::ContainsLabel(const Stmt* stmt) {
  if (stmt == nullptr) {
    return false;
  }

  // 比如 if(0){label:...} goto label;
  // 依旧需要生成代码
  if (stmt->Kind() == AstNodeType::kLabelStmt ||
      stmt->Kind() == AstNodeType::kCaseStmt ||
      stmt->Kind() == AstNodeType::kDefaultStmt ||
      stmt->Kind() == AstNodeType::kSwitchStmt) {
    return true;
  }

  for (const auto& item : stmt->Children()) {
    if (ContainsLabel(item)) {
      return true;
    }
  }

  return false;
}

void CodeGen::EmitBranchThroughCleanup(llvm::BasicBlock* dest) {
  if (!HaveInsertPoint()) {
    return;
  }

  Builder.CreateBr(dest);
  Builder.ClearInsertionPoint();
}

llvm::BasicBlock* CodeGen::GetBasicBlockForLabel(const LabelStmt* label) {
  auto& bb{labels_[label]};
  if (bb) {
    return bb;
  }

  return bb = CreateBasicBlock(label->GetName());
}

bool CodeGen::IsCheapEnoughToEvaluateUnconditionally(const Expr* expr) {
  return expr->Kind() == AstNodeType::kConstantExpr;
}

llvm::AllocaInst* CodeGen::CreateEntryBlockAlloca(llvm::Type* type,
                                                  std::int32_t align,
                                                  const std::string& name) {
  (void)name;
#ifdef NDEBUG
  auto ptr{new llvm::AllocaInst{type, 0, "", alloc_insert_point_}};
#else
  auto ptr{new llvm::AllocaInst{type, 0, name, alloc_insert_point_}};
#endif

  ptr->setAlignment(llvm::MaybeAlign{static_cast<std::uint64_t>(align)});
  return ptr;
}

llvm::Value* CodeGen::GetPtr(const AstNode* node) {
  if (node->Kind() == AstNodeType::kObjectExpr) {
    auto obj{dynamic_cast<const ObjectExpr*>(node)};
    is_volatile_ = obj->GetQualType().IsVolatile();

    if (obj->IsGlobalVar() || obj->IsLocalStaticVar()) {
      return obj->GetGlobalPtr();
    } else {
      return obj->GetLocalPtr();
    }
  } else if (node->Kind() == AstNodeType::kIdentifierExpr) {
    // 函数指针
    node->Accept(*this);
    return result_;
  } else if (node->Kind() == AstNodeType::kUnaryOpExpr) {
    auto unary{dynamic_cast<const UnaryOpExpr*>(node)};
    assert(unary->GetOp() == Tag::kStar);
    unary->GetExpr()->Accept(*this);
    return result_;
  } else if (node->Kind() == AstNodeType::kBinaryOpExpr) {
    auto binary{dynamic_cast<const BinaryOpExpr*>(node)};
    if (binary->GetOp() == Tag::kPeriod) {
      auto lhs_ptr{GetPtr(binary->GetLHS())};
      auto obj{dynamic_cast<const ObjectExpr*>(binary->GetRHS())};
      assert(obj != nullptr);

      // 注意, volatile 限定的结构体或联合体类型, 其成员
      // 会获取其所属类型的限定(当通过 . 或 -> 运算符时)
      if (obj->GetQualType().IsVolatile()) {
        is_volatile_ = true;
      }

      if (obj->GetBitFieldWidth()) {
        is_bit_field_ = true;
        bit_field_ = const_cast<ObjectExpr*>(obj);
      }

      auto indexs{obj->GetIndexs()};
      for (const auto& [type, index] : indexs) {
        if (type->IsStructTy()) {
          lhs_ptr = Builder.CreateStructGEP(lhs_ptr, index);
        } else {
          lhs_ptr = Builder.CreateBitCast(
              lhs_ptr,
              type->StructGetMemberType(index)->GetLLVMType()->getPointerTo());
        }
      }

      if (is_bit_field_) {
        if (obj->GetType()->IsBoolTy()) {
          lhs_ptr = Builder.CreateBitCast(lhs_ptr,
                                          Builder.getInt8Ty()->getPointerTo());
        } else {
          lhs_ptr = Builder.CreateBitCast(lhs_ptr,
                                          Builder.getInt32Ty()->getPointerTo());
        }
      }

      return lhs_ptr;
    } else if (binary->GetOp() == Tag::kEqual) {
      SetIgnoreAssignResult();
      binary->Accept(*this);
      return result_;
    }
  }

  assert(false);
  return nullptr;
}

void CodeGen::PushBlock(llvm::BasicBlock* break_stack,
                        llvm::BasicBlock* continue_block) {
  break_continue_stack_.push({break_stack, continue_block});
}

void CodeGen::PopBlock() { break_continue_stack_.pop(); }

bool CodeGen::TestAndClearIgnoreAssignResult() {
  auto ret{ignore_assign_result_};
  ignore_assign_result_ = false;
  return ret;
}

void CodeGen::SetIgnoreAssignResult() { ignore_assign_result_ = true; }

void CodeGen::TryEmitLocation(const AstNode* node) {
  if (debug_info_) {
    debug_info_->EmitLocation(node);
  }
}

void CodeGen::TryEmitFuncStart(const FuncDef* node) {
  if (debug_info_) {
    debug_info_->EmitFuncStart(node);
  }
}

void CodeGen::TryEmitFuncEnd() {
  if (debug_info_) {
    debug_info_->EmitFuncEnd();
  }
}

void CodeGen::TryEmitParamVar(const std::string& name, Type* type,
                              llvm::AllocaInst* ptr, const Location& loc) {
  if (debug_info_) {
    debug_info_->EmitParamVar(name, type, ptr, loc);
  }
}

void CodeGen::TryEmitLocalVar(const Declaration* node) {
  if (debug_info_) {
    debug_info_->EmitLocalVar(node);
  }
}

void CodeGen::TryEmitGlobalVar(const Declaration* node) {
  if (debug_info_) {
    debug_info_->EmitGlobalVar(node);
  }
}

void CodeGen::Visit(const TranslationUnit* node) {
  TryEmitLocation(node);

  for (const auto& item : node->GetExtDecl()) {
    item->Accept(*this);
  }
}

void CodeGen::Visit(const Declaration* node) {
  TryEmitLocation(node);

  // 对于函数声明, 当函数调用或者定义时处理
  if (!node->IsObjDecl()) {
    return;
  } else if (node->IsObjDeclInGlobalOrLocalStatic()) {
    if (node->IsObjDecl()) {
      auto obj{node->GetIdent()->ToObjectExpr()};
      // 对于非 static 的全局变量直接生成
      // 其他的等到调用 GetGlobalPtr 是再生成
      if (obj->IsGlobalVar() && !obj->IsStatic()) {
        CreateGlobalVar(obj);
      }
    }

    TryEmitGlobalVar(node);
    return;
  } else {
    DealLocaleDecl(node);
  }
}

void CodeGen::Visit(const FuncDef* node) {
  StartFunction(node);

  TryEmitFuncStart(node);
  TryEmitLocation(nullptr);

  auto iter{std::begin(node->GetFuncType()->FuncGetParams())};
  for (auto&& arg : func_->args()) {
    auto obj{*iter};
    auto type{obj->GetType()};
    auto name{obj->GetName()};

    auto ptr{
        CreateEntryBlockAlloca(type->GetLLVMType(), (*iter)->GetAlign(), name)};
    (*iter)->SetLocalPtr(ptr);

    TryEmitParamVar(name, type, ptr, obj->GetLoc());
    // 将参数的值保存到分配的内存中
    Builder.CreateStore(&arg, ptr, is_volatile_);
    ++iter;
  }

  TryEmitLocation(node);

  if (node->GetFuncType()->FuncGetName() == "main") {
    Builder.CreateStore(Builder.getInt32(0), return_value_);
  }

  EmitStmt(node->GetBody());
  FinishFunction(node);

  TryEmitFuncEnd();
}

void CodeGen::DealLocaleDecl(const Declaration* node) {
  auto obj{node->GetObject()};
  auto type{obj->GetType()};
  auto name{obj->GetName()};

  auto ptr{CreateEntryBlockAlloca(type->GetLLVMType(), obj->GetAlign(), name)};
  obj->SetLocalPtr(ptr);

  is_volatile_ = obj->GetQualType().IsVolatile();

  TryEmitLocalVar(node);

  if (node->HasLocalInit()) {
    if (type->IsScalarTy()) {
      auto init{node->GetLocalInits()};
      assert(std::size(init) == 1);

      init.front().GetExpr()->Accept(*this);
      Builder.CreateStore(result_, obj->GetLocalPtr(), is_volatile_);
      is_volatile_ = false;
    } else if (type->IsAggregateTy()) {
      InitLocalAggregate(node);
    } else {
      assert(false);
    }
  } else if (node->HasConstantInit()) {
    if (ptr->getType() != Builder.getInt8PtrTy()) {
      result_ = Builder.CreateBitCast(ptr, Builder.getInt8PtrTy());
    }

    auto align{llvm::MaybeAlign{static_cast<std::uint64_t>(obj->GetAlign())}};
    Builder.CreateMemCpy(result_, align, node->GetConstant(), align,
                         obj->GetType()->GetWidth(), is_volatile_);
    is_volatile_ = false;
  }
}

void CodeGen::InitLocalAggregate(const Declaration* node) {
  EnsureInsertPoint();

  auto obj{node->GetObject()};
  auto width{obj->GetType()->GetWidth()};

  result_ = Builder.CreateBitCast(obj->GetLocalPtr(), Builder.getInt8PtrTy());
  Builder.CreateMemSet(
      result_, Builder.getInt8(0), width,
      llvm::MaybeAlign{static_cast<std::uint64_t>(obj->GetAlign())},
      is_volatile_);

  for (const auto& item : node->GetLocalInits()) {
    Load_Struct_Obj();
    item.GetExpr()->Accept(*this);
    Finish_Load();
    auto value{result_};

    llvm::Value* ptr{obj->GetLocalPtr()};
    Type* member_type{};
    std::int8_t bit_field_begin{}, bit_field_width{};
    for (const auto& [type, index, begin, width] : item.GetIndexs()) {
      bit_field_begin = begin;
      bit_field_width = width;

      if (type->IsArrayTy() && !width) {
        member_type = type->ArrayGetElementType().GetType();
        ptr = Builder.CreateInBoundsGEP(
            ptr, {Builder.getInt64(0), Builder.getInt64(index)});
      } else if (type->IsStructTy()) {
        member_type = type->StructGetMemberType(index).GetType();
        ptr = Builder.CreateStructGEP(ptr, index);
      } else if (type->IsUnionTy()) {
        member_type = type->StructGetMemberType(index).GetType();
        ptr = Builder.CreateBitCast(ptr,
                                    member_type->GetLLVMType()->getPointerTo());
      } else {
        member_type = type;
        break;
      }
    }

    if (member_type && bit_field_width) {
      auto size{member_type->IsBoolTy() ? 8 : 32};

      if (member_type->IsBoolTy()) {
        ptr = Builder.CreateBitCast(ptr, Builder.getInt8PtrTy());
      } else {
        ptr = Builder.CreateBitCast(ptr, Builder.getInt32Ty()->getPointerTo());
      }

      result_ = Builder.CreateLoad(ptr, is_volatile_);
      result_ = GetBitField(result_, size, bit_field_width, bit_field_begin);

      value = Builder.CreateShl(value, bit_field_begin);
      value = CastTo(value, Builder.getInt32Ty(),
                     item.GetExpr()->GetType()->IsUnsigned());
      value = Builder.CreateOr(result_, value);
    }

    result_ = Builder.CreateStore(value, ptr, is_volatile_);
  }

  is_volatile_ = false;
}

void CodeGen::StartFunction(const FuncDef* node) {
  node->GetIdent()->Accept(*this);
  func_ = llvm::cast<llvm::Function>(result_);

  auto func_name{node->GetName()};
  auto func_type{node->GetFuncType()};

  if (node->GetLinkage() != Linkage::kInternal) {
    func_->setDSOLocal(true);
  }

  func_->addFnAttr(llvm::Attribute::NoUnwind);
  func_->addFnAttr(llvm::Attribute::StackProtectStrong);
  func_->addFnAttr(llvm::Attribute::UWTable);

  if (OptimizationLevel == OptLevel::kO0) {
    func_->addFnAttr(llvm::Attribute::NoInline);
    func_->addFnAttr(llvm::Attribute::OptimizeNone);
  }

  auto entry{CreateBasicBlock("entry", func_)};

  auto undef{llvm::UndefValue::get(Builder.getInt32Ty())};
  alloc_insert_point_ =
      new llvm::BitCastInst{undef, Builder.getInt32Ty(), "", entry};

  return_block_ = CreateBasicBlock("return");
  return_value_ = nullptr;

  auto return_type{func_type->FuncGetReturnType()};
  if (!return_type->IsVoidTy()) {
    return_value_ = CreateEntryBlockAlloca(return_type->GetLLVMType(),
                                           return_type->GetAlign(), "ret.val");
  }

  Builder.SetInsertPoint(entry);
}

void CodeGen::FinishFunction(const FuncDef* node) {
  auto func{Module->getFunction(node->GetName())};

  EmitReturnBlock();
  EmitFunctionEpilog();

  auto ptr{alloc_insert_point_};
  alloc_insert_point_ = nullptr;
  ptr->eraseFromParent();

  labels_.clear();

  // 验证生成的代码, 检查一致性
  llvm::verifyFunction(*func);
}

void CodeGen::EmitReturnBlock() {
  auto bb{Builder.GetInsertBlock()};

  if (bb) {
    assert(!bb->getTerminator());
    // e.g.
    // void print(char *s) {
    //    printf("Testing %s ... ", s);
    //    fflush(stdout);
    //}
    if (bb->empty() || return_block_->use_empty()) {
      return_block_->replaceAllUsesWith(bb);
      delete return_block_;
      return;
    }
  }

  EmitBlock(return_block_);
}

void CodeGen::EmitFunctionEpilog() {
  llvm::Value* value{};

  if (return_value_) {
    value = Builder.CreateLoad(return_value_);
  }

  if (value) {
    Builder.CreateRet(value);
  } else {
    Builder.CreateRetVoid();
  }
}

}  // namespace kcc
