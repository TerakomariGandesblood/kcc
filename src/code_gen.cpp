//
// Created by kaiser on 2019/11/2.
//

#include "code_gen.h"

#include <llvm/ADT/Optional.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>

#include "error.h"
#include "util.h"

namespace kcc {

CodeGen::CodeGen(const std::string& file_name) {
  Module = std::make_unique<llvm::Module>(file_name, Context);
  DataLayout = llvm::DataLayout{Module.get()};

  // 初始化
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();

  // 获取当前计算机的目标三元组
  auto target_triple{llvm::sys::getDefaultTargetTriple()};

  std::string error;
  auto target{llvm::TargetRegistry::lookupTarget(target_triple, error)};

  if (!target) {
    Error(error);
  }

  // 使用通用CPU, 生成位置无关目标文件
  std::string cpu("generic");
  std::string features;
  llvm::TargetOptions opt;
  llvm::Optional<llvm::Reloc::Model> rm{llvm::Reloc::Model::PIC_};
  TargetMachine = std::unique_ptr<llvm::TargetMachine>{
      target->createTargetMachine(target_triple, cpu, features, opt, rm)};

  // 配置模块以指定目标机器和数据布局, 这不是必需的, 但有利于优化
  Module->setDataLayout(TargetMachine->createDataLayout());
  Module->setTargetTriple(target_triple);
}

void CodeGen::GenCode(const TranslationUnit* root) { root->Accept(*this); }

/*
 * ++ --
 * + - ~
 * !
 * * &
 */
void CodeGen::Visit(const UnaryOpExpr& node) {
  auto is_unsigned{node.expr_->GetType()->IsUnsigned()};

  switch (node.op_) {
    case Tag::kPlusPlus: {
      auto lhs_ptr{GetPtr(*node.expr_)};
      auto lhs_value{Builder.CreateAlignedLoad(
          lhs_ptr, DataLayout.getABITypeAlignment(lhs_ptr->getType()))};
      auto rhs_value{AddOp(lhs_value, Builder.getInt32(1), is_unsigned)};
      result_ = Assign(lhs_ptr, rhs_value);
    } break;
    case Tag::kPostfixPlusPlus: {
      auto lhs_ptr{GetPtr(*node.expr_)};
      auto lhs_value{Builder.CreateAlignedLoad(
          lhs_ptr, DataLayout.getABITypeAlignment(lhs_ptr->getType()))};
      auto rhs_value{AddOp(lhs_value, Builder.getInt32(1), is_unsigned)};
      Assign(lhs_ptr, rhs_value);
      result_ = lhs_value;
    } break;
    case Tag::kMinusMinus: {
      auto lhs_ptr{GetPtr(*node.expr_)};
      auto lhs_value{Builder.CreateAlignedLoad(
          lhs_ptr, DataLayout.getABITypeAlignment(lhs_ptr->getType()))};
      auto rhs_value{SubOp(lhs_value, Builder.getInt32(1), is_unsigned)};
      result_ = Assign(lhs_ptr, rhs_value);
    } break;
    case Tag::kPostfixMinusMinus: {
      auto lhs_ptr{GetPtr(*node.expr_)};
      auto lhs_value{Builder.CreateAlignedLoad(
          lhs_ptr, DataLayout.getABITypeAlignment(lhs_ptr->getType()))};
      auto rhs_value{SubOp(lhs_value, Builder.getInt32(1), is_unsigned)};
      Assign(lhs_ptr, rhs_value);
      result_ = lhs_value;
    } break;
    case Tag::kPlus:
      node.expr_->Accept(*this);
      break;
    case Tag::kMinus:
      node.expr_->Accept(*this);
      result_ = NegOp(result_, is_unsigned);
      break;
    case Tag::kTilde:
      node.expr_->Accept(*this);
      result_ = Builder.CreateNot(result_);
      break;
    case Tag::kExclaim:
      node.expr_->Accept(*this);
      result_ = LogicNotOp(result_);
      break;
    case Tag::kStar: {
      auto ptr{GetPtr(*node.expr_)};
      result_ = Builder.CreateAlignedLoad(
          ptr, DataLayout.getABITypeAlignment(
                   ptr->getType()->getPointerElementType()));
    } break;
    case Tag::kAmp:
      result_ = GetPtr(*node.expr_);
      break;
    default:
      assert(false);
  }
}

void CodeGen::Visit(const BinaryOpExpr& node) {
  switch (node.op_) {
    case Tag::kPipePipe:
      result_ = LogicOrOp(node);
      return;
    case Tag::kAmpAmp:
      result_ = LogicAndOp(node);
      return;
    case Tag::kEqual:
      result_ = AssignOp(node);
      return;
    case Tag::kArrow:
      result_ = Builder.CreateLoad(GetPtr(node));
      return;
    default:
      break;
  }

  node.lhs_->Accept(*this);
  auto lhs{result_};
  node.rhs_->Accept(*this);
  auto rhs{result_};

  bool is_unsigned{node.GetType()->IsUnsigned()};

  switch (node.op_) {
    case Tag::kPlus:
      result_ = AddOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kMinus:
      result_ = SubOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kStar:
      result_ = MulOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kSlash:
      result_ = DivOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kPercent:
      result_ = ModOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kPipe:
      result_ = OrOp(lhs, rhs);
      break;
    case Tag::kAmp:
      result_ = AndOp(lhs, rhs);
      break;
    case Tag::kCaret:
      result_ = XorOp(lhs, rhs);
      break;
    case Tag::kLessLess:
      result_ = ShlOp(lhs, rhs);
      break;
    case Tag::kGreaterGreater:
      result_ = ShrOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kEqualEqual:
      result_ = EqualOp(lhs, rhs);
      break;
    case Tag::kExclaimEqual:
      result_ = NotEqualOp(lhs, rhs);
      break;
    case Tag::kLess:
      result_ = LessOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kLessEqual:
      result_ = LessEqualOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kGreater:
      result_ = GreaterOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kGreaterEqual:
      result_ = GreaterEqualOp(lhs, rhs, is_unsigned);
      break;
    case Tag::kComma:
      result_ = rhs;
      break;
    default:
      assert(false);
  }
}

void CodeGen::Visit(const ConditionOpExpr& node) {
  node.cond_->Accept(*this);
  auto cond{CastToBool(result_)};

  node.lhs_->Accept(*this);
  auto lhs{result_};
  node.rhs_->Accept(*this);
  auto rhs{result_};

  result_ = Builder.CreateSelect(cond, lhs, rhs);
}

void CodeGen::Visit(const TypeCastExpr& node) {
  node.expr_->Accept(*this);
  result_ = CastTo(result_, node.to_->GetLLVMType(),
                   node.to_->IsIntegerTy() ? node.to_->IsUnsigned() : false);
}

// 常量用 ConstantFP / ConstantInt 类表示
// 在 LLVM IR 中, 常量都是唯一且共享的
void CodeGen::Visit(const Constant& node) {
  auto type{node.GetType()->GetLLVMType()};

  if (type->isIntegerTy()) {
    result_ = llvm::ConstantInt::get(
        Context, llvm::APInt(type->getIntegerBitWidth(), node.integer_val_,
                             !node.GetType()->IsUnsigned()));
  } else if (type->isFloatingPointTy()) {
    result_ =
        llvm::ConstantFP::get(Context, llvm::APFloat(node.float_point_val_));
  } else {
    result_ = Builder.CreateGlobalStringPtr(node.str_val_);
  }
}

void CodeGen::Visit(const Enumerator& node) {
  result_ = llvm::ConstantInt::get(Context, llvm::APInt(32, node.val_, true));
}

void CodeGen::Visit(const CompoundStmt& node) {
  for (auto& item : node.stmts_) {
    item->Accept(*this);
  }
}

void CodeGen::Visit(const IfStmt& node) {
  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto then_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto else_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  need_bool_ = true;
  node.cond_->Accept(*this);
  need_bool_ = false;
  result_ = CastToBool(result_);

  if (node.else_block_) {
    Builder.CreateCondBr(result_, then_block, else_block);
  } else {
    else_block->removeFromParent();
    Builder.CreateCondBr(result_, then_block, after_block);
  }

  Builder.SetInsertPoint(then_block);
  node.then_block_->Accept(*this);
  if (!HasBrOrReturn()) {
    Builder.CreateBr(after_block);
  }

  if (node.else_block_) {
    Builder.SetInsertPoint(else_block);
    node.else_block_->Accept(*this);

    if (!HasBrOrReturn()) {
      Builder.CreateBr(after_block);
    }
  }

  Builder.SetInsertPoint(after_block);
}

void CodeGen::Visit(const ReturnStmt& node) {
  if (node.expr_) {
    node.expr_->Accept(*this);
    Builder.CreateRet(result_);
  } else {
    Builder.CreateRetVoid();
    has_br_or_return_.top().second = true;
  }
}

void CodeGen::Visit(const LabelStmt&) {}

void CodeGen::Visit(const FuncCallExpr& node) {
  node.callee_->Accept(*this);
  auto callee{result_};

  std::vector<llvm::Value*> values;
  for (auto& item : node.args_) {
    item->Accept(*this);
    values.push_back(result_);
  }

  Builder.CreateCall(callee, values);
}

void CodeGen::Visit(const Identifier& node) {
  auto type{node.GetType()};
  assert(type->IsFunctionTy());

  auto func = Module->getFunction(node.GetName());
  if (!func) {
    func = llvm::Function::Create(
        llvm::cast<llvm::FunctionType>(type->GetLLVMType()),
        node.GetLinkage() == kInternal ? llvm::Function::InternalLinkage
                                       : llvm::Function::ExternalLinkage,
        node.GetName(), Module.get());
  }

  result_ = func;
}

void CodeGen::Visit(const Object& node) {
  if (!node.in_global_) {
    result_ = Builder.CreateAlignedLoad(node.local_ptr_, node.GetAlign());
  } else {
    assert(false);
  }
}

void CodeGen::Visit(const TranslationUnit& node) {
  for (auto& item : node.ext_decls_) {
    item->Accept(*this);
  }
}

void CodeGen::Visit(const Declaration& node) {
  if (node.IsObj()) {
    auto obj{dynamic_cast<Object*>(node.ident_)};
    auto type{node.ident_->GetType()};

    if (!obj->in_global_) {
      if (Builder.GetInsertBlock() == nullptr) {
        Error(node.ident_->GetToken(), "fuck");
      }
      obj->local_ptr_ = CreateEntryBlockAlloca(
          Builder.GetInsertBlock()->getParent(), type->GetLLVMType(),
          obj->GetAlign(), obj->GetName());
    }
  }
}

void CodeGen::Visit(const FuncDef& node) {
  auto func_name{node.GetName()};
  auto type{node.GetFuncType()};
  auto func{Module->getFunction(func_name)};

  if (!func) {
    func = llvm::Function::Create(
        llvm::cast<llvm::FunctionType>(type->GetLLVMType()),
        node.GetIdent()->GetLinkage() == kInternal
            ? llvm::Function::InternalLinkage
            : llvm::Function::ExternalLinkage,
        node.GetIdent()->GetName(), Module.get());
  }

  auto entry{llvm::BasicBlock::Create(Context, "", func)};
  Builder.SetInsertPoint(entry);

  node.body_->Accept(*this);

  Builder.CreateRet(llvm::ConstantInt::get(Context, llvm::APInt(32, 0, true)));

  // 验证生成的代码, 检查一致性
  llvm::verifyFunction(*func);
}

void CodeGen::Visit(const ExprStmt& node) {
  if (node.expr_) {
    node.expr_->Accept(*this);
  }
}

void CodeGen::Visit(const WhileStmt& node) {
  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto cond_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto loop_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  Builder.CreateBr(cond_block);

  Builder.SetInsertPoint(cond_block);
  need_bool_ = true;
  node.cond_->Accept(*this);
  need_bool_ = false;
  result_ = CastToBool(result_);
  Builder.CreateCondBr(result_, loop_block, after_block);

  Builder.SetInsertPoint(loop_block);

  PushBlock(cond_block, after_block);
  node.block_->Accept(*this);
  PopBlock();

  if (!HasBrOrReturn()) {
    Builder.CreateBr(cond_block);
  }

  Builder.SetInsertPoint(after_block);
}

void CodeGen::Visit(const DoWhileStmt& node) {
  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto loop_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto cond_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  Builder.CreateBr(loop_block);

  Builder.SetInsertPoint(loop_block);
  PushBlock(cond_block, after_block);

  node.block_->Accept(*this);
  if (!HasBrOrReturn()) {
    Builder.CreateBr(cond_block);
  }

  PopBlock();

  Builder.SetInsertPoint(cond_block);
  need_bool_ = true;
  node.cond_->Accept(*this);
  need_bool_ = false;
  result_ = CastToBool(result_);
  Builder.CreateCondBr(result_, loop_block, after_block);

  Builder.SetInsertPoint(after_block);
}

void CodeGen::Visit(const ForStmt& node) {
  if (node.init_) {
    node.init_->Accept(*this);
  } else if (node.decl_) {
    node.decl_->Accept(*this);
  }

  if (!node.inc_ && !node.cond_) {
    VisitForNoIncCond(node);
  } else if (!node.inc_ && node.cond_) {
    VisitForNoInc(node);
  } else if (node.inc_ && !node.cond_) {
    VisitForNoCond(node);
  } else {
    auto parent_func{Builder.GetInsertBlock()->getParent()};
    auto cond_block{llvm::BasicBlock::Create(Context, "", parent_func)};
    auto loop_block{llvm::BasicBlock::Create(Context, "", parent_func)};
    auto inc_block{llvm::BasicBlock::Create(Context, "", parent_func)};
    auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

    Builder.CreateBr(cond_block);
    Builder.SetInsertPoint(cond_block);

    need_bool_ = true;
    node.cond_->Accept(*this);
    need_bool_ = false;
    result_ = CastToBool(result_);
    Builder.CreateCondBr(result_, loop_block, after_block);

    Builder.SetInsertPoint(loop_block);

    PushBlock(cond_block, after_block);
    node.block_->Accept(*this);
    PopBlock();

    Builder.CreateBr(inc_block);

    Builder.SetInsertPoint(inc_block);

    node.inc_->Accept(*this);
    if (!HasBrOrReturn()) {
      Builder.CreateBr(cond_block);
    }

    Builder.SetInsertPoint(after_block);
  }
}

void CodeGen::Visit(const CaseStmt&) {}

void CodeGen::Visit(const DefaultStmt&) {}

void CodeGen::Visit(const SwitchStmt&) {}

void CodeGen::Visit(const BreakStmt&) {
  if (std::empty(break_stack_) || break_stack_.top() == nullptr) {
    Error("Cannot use break here");
  } else {
    Builder.CreateBr(break_stack_.top());
    has_br_or_return_.top().first = true;
  }
}

void CodeGen::Visit(const ContinueStmt&) {
  if (std::empty(continue_stack_) || continue_stack_.top() == nullptr) {
    Error("Cannot use continue here");
  } else {
    Builder.CreateBr(continue_stack_.top());
    has_br_or_return_.top().first = true;
  }
}

void CodeGen::Visit(const GotoStmt&) {}

llvm::AllocaInst* CodeGen::CreateEntryBlockAlloca(llvm::Function* parent,
                                                  llvm::Type* type,
                                                  std::int32_t align,
                                                  const std::string& name) {
  // 指向入口块开始的位置
  llvm::IRBuilder<> temp{&parent->getEntryBlock(),
                         parent->getEntryBlock().begin()};

  auto ptr{temp.CreateAlloca(type, nullptr, name)};
  ptr->setAlignment(align);

  return ptr;
}

llvm::Value* CodeGen::AddOp(llvm::Value* lhs, llvm::Value* rhs,
                            bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return Builder.CreateAdd(lhs, rhs);
    } else {
      return Builder.CreateNSWAdd(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return Builder.CreateFAdd(lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    // 进行地址计算, 第二个参数是偏移量列表
    return llvm::GetElementPtrInst::CreateInBounds(lhs, {rhs}, "",
                                                   Builder.GetInsertBlock());
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::SubOp(llvm::Value* lhs, llvm::Value* rhs,
                            bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return Builder.CreateSub(lhs, rhs);
    } else {
      return Builder.CreateNSWSub(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return Builder.CreateFSub(lhs, rhs);
  } else if (IsPointerTy(lhs) && IsIntegerTy(rhs)) {
    return llvm::GetElementPtrInst::CreateInBounds(
        lhs, {Builder.CreateNeg(rhs)}, "", Builder.GetInsertBlock());
  } else if (IsPointerTy(lhs) && IsPointerTy(rhs)) {
    auto type{lhs->getType()->getPointerElementType()};

    lhs = CastTo(lhs, Builder.getInt64Ty(), false);
    rhs = CastTo(rhs, Builder.getInt64Ty(), false);

    auto value{SubOp(lhs, rhs, false)};
    return DivOp(value, Builder.getInt64(DataLayout.getTypeAllocSize(type)),
                 false);
  } else {
    assert(false);
    return nullptr;
  }
}

bool CodeGen::IsArithmeticTy(llvm::Value* value) const {
  return IsIntegerTy(value) || IsFloatingPointTy(value);
}

bool CodeGen::IsPointerTy(llvm::Value* value) const {
  return value->getType()->isPointerTy();
}

bool CodeGen::IsIntegerTy(llvm::Value* value) const {
  return value->getType()->isIntegerTy();
}

bool CodeGen::IsFloatingPointTy(llvm::Value* value) const {
  return value->getType()->isFloatingPointTy();
}

llvm::Value* CodeGen::MulOp(llvm::Value* lhs, llvm::Value* rhs,
                            bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return Builder.CreateMul(lhs, rhs);
    } else {
      return Builder.CreateNSWMul(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return Builder.CreateFMul(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::DivOp(llvm::Value* lhs, llvm::Value* rhs,
                            bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return Builder.CreateUDiv(lhs, rhs);
    } else {
      return Builder.CreateSDiv(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return Builder.CreateFDiv(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::CastTo(llvm::Value* value, llvm::Type* to,
                             bool is_unsigned) {
  if (to->isIntegerTy(1)) {
    return CastToBool(value);
  } else if (value->getType()->isIntegerTy(1) && to->isIntegerTy()) {
    return Builder.CreateZExtOrTrunc(value, to);
  }

  if (IsIntegerTy(value) && to->isIntegerTy()) {
    if (is_unsigned) {
      return Builder.CreateZExtOrTrunc(value, to);
    } else {
      return Builder.CreateSExtOrTrunc(value, to);
    }
  } else if (IsIntegerTy(value) && to->isFloatingPointTy()) {
    if (is_unsigned) {
      return Builder.CreateUIToFP(value, to);
    } else {
      return Builder.CreateSIToFP(value, to);
    }
  } else if (IsFloatingPointTy(value) && to->isIntegerTy()) {
    if (is_unsigned) {
      return Builder.CreateFPToUI(value, to);
    } else {
      return Builder.CreateFPToSI(value, to);
    }
  } else if (IsFloatingPointTy(value) && to->isFloatingPointTy()) {
    if (FloatPointRank(value->getType()) > FloatPointRank(to)) {
      return Builder.CreateFPTrunc(value, to);
    } else {
      return Builder.CreateFPExt(value, to);
    }
  } else if (to->isVoidTy()) {
    return value;
  } else if (value->getType() == to) {
    return value;
  } else {
    Error("{} to {}", LLVMTypeToStr(value->getType()), LLVMTypeToStr(to));
  }
}

llvm::Value* CodeGen::ModOp(llvm::Value* lhs, llvm::Value* rhs,
                            bool is_unsigned) {
  if (is_unsigned) {
    return Builder.CreateURem(lhs, rhs);
  } else {
    return Builder.CreateSRem(lhs, rhs);
  }
}

llvm::Value* CodeGen::OrOp(llvm::Value* lhs, llvm::Value* rhs) {
  return Builder.CreateOr(lhs, rhs);
}

llvm::Value* CodeGen::AndOp(llvm::Value* lhs, llvm::Value* rhs) {
  return Builder.CreateAnd(lhs, rhs);
}

llvm::Value* CodeGen::XorOp(llvm::Value* lhs, llvm::Value* rhs) {
  return Builder.CreateXor(lhs, rhs);
}

llvm::Value* CodeGen::ShlOp(llvm::Value* lhs, llvm::Value* rhs) {
  return Builder.CreateShl(lhs, rhs);
}

llvm::Value* CodeGen::ShrOp(llvm::Value* lhs, llvm::Value* rhs,
                            bool is_unsigned) {
  if (is_unsigned) {
    return Builder.CreateLShr(lhs, rhs);
  } else {
    return Builder.CreateAShr(lhs, rhs);
  }
}

llvm::Value* CodeGen::LessEqualOp(llvm::Value* lhs, llvm::Value* rhs,
                                  bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      auto value{Builder.CreateICmpULE(lhs, rhs)};
      return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
    } else {
      auto value{Builder.CreateICmpSLE(lhs, rhs)};
      return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
    }
  } else if (IsFloatingPointTy(lhs)) {
    auto value{Builder.CreateFCmpOLE(lhs, rhs)};
    return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
  } else if (IsPointerTy(lhs)) {
    auto value{Builder.CreateICmpULE(lhs, rhs)};
    return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::LessOp(llvm::Value* lhs, llvm::Value* rhs,
                             bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      auto value{Builder.CreateICmpULT(lhs, rhs)};
      return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
    } else {
      auto value{Builder.CreateICmpSLT(lhs, rhs)};
      return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
    }
  } else if (IsFloatingPointTy(lhs)) {
    auto value{Builder.CreateFCmpOLT(lhs, rhs)};
    return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
  } else if (IsPointerTy(lhs)) {
    auto value{Builder.CreateICmpULT(lhs, rhs)};
    return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::GreaterEqualOp(llvm::Value* lhs, llvm::Value* rhs,
                                     bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      auto value{Builder.CreateICmpUGE(lhs, rhs)};
      return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
    } else {
      auto value{Builder.CreateICmpSGE(lhs, rhs)};
      return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
    }
  } else if (IsFloatingPointTy(lhs)) {
    auto value{Builder.CreateFCmpOGE(lhs, rhs)};
    return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
  } else if (IsPointerTy(lhs)) {
    auto value{Builder.CreateICmpUGE(lhs, rhs)};
    return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::GreaterOp(llvm::Value* lhs, llvm::Value* rhs,
                                bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      auto value{Builder.CreateICmpUGT(lhs, rhs)};
      return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
    } else {
      auto value{Builder.CreateICmpSGT(lhs, rhs)};
      return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
    }
  } else if (IsFloatingPointTy(lhs)) {
    auto value{Builder.CreateFCmpOGT(lhs, rhs)};
    return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
  } else if (IsPointerTy(lhs)) {
    auto value{Builder.CreateICmpUGT(lhs, rhs)};
    return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::EqualOp(llvm::Value* lhs, llvm::Value* rhs) {
  if (IsIntegerTy(lhs) || IsPointerTy(lhs)) {
    auto value{Builder.CreateICmpEQ(lhs, rhs)};
    return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
  } else if (IsFloatingPointTy(lhs)) {
    auto value{Builder.CreateFCmpOEQ(lhs, rhs)};
    return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::NotEqualOp(llvm::Value* lhs, llvm::Value* rhs) {
  if (IsIntegerTy(lhs) || IsPointerTy(lhs)) {
    auto value{Builder.CreateICmpNE(lhs, rhs)};
    return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
  } else if (IsFloatingPointTy(lhs)) {
    auto value{Builder.CreateFCmpONE(lhs, rhs)};
    return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::CastToBool(llvm::Value* value) {
  if (value->getType()->isIntegerTy(1)) {
    return value;
  }

  if (IsIntegerTy(value) || IsPointerTy(value)) {
    return Builder.CreateICmpNE(value, GetZero(value->getType()));
  } else if (IsFloatingPointTy(value)) {
    return Builder.CreateFCmpONE(value, GetZero(value->getType()));
  } else {
    Error("{}", LLVMTypeToStr(value->getType()));
  }
}

bool CodeGen::IsFloatTy(llvm::Value* value) const {
  return value->getType()->isFloatTy();
}

bool CodeGen::IsDoubleTy(llvm::Value* value) const {
  return value->getType()->isDoubleTy();
}

bool CodeGen::IsLongDoubleTy(llvm::Value* value) const {
  return value->getType()->isX86_FP80Ty();
}

llvm::Value* CodeGen::GetZero(llvm::Type* type) {
  if (type->isIntegerTy()) {
    return llvm::ConstantInt::get(Context,
                                  llvm::APInt(type->getIntegerBitWidth(), 0));
  } else if (type->isFloatTy()) {
    return llvm::ConstantFP::get(Context, llvm::APFloat(0.0f));
  } else if (type->isDoubleTy()) {
    return llvm::ConstantFP::get(Context, llvm::APFloat(0.0));
  } else if (type->isX86_FP80Ty()) {
    return llvm::ConstantFP::get(llvm::Type::getX86_FP80Ty(Context),
                                 llvm::APFloat(0.0));
  } else if (type->isPointerTy()) {
    return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
  } else {
    assert(false);
    return nullptr;
  }
}

std::int32_t CodeGen::FloatPointRank(llvm::Type* type) const {
  if (type->isFloatTy()) {
    return 0;
  } else if (type->isDoubleTy()) {
    return 1;
  } else if (type->isX86_FP80Ty()) {
    return 2;
  } else {
    assert(false);
    return 0;
  }
}

llvm::Value* CodeGen::LogicOrOp(const BinaryOpExpr& node) {
  auto block{Builder.GetInsertBlock()};
  auto parent_func{block->getParent()};
  auto rhs_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  node.lhs_->Accept(*this);
  result_ = CastToBool(result_);
  Builder.CreateCondBr(result_, after_block, rhs_block);

  Builder.SetInsertPoint(rhs_block);
  node.rhs_->Accept(*this);
  result_ = CastToBool(result_);
  Builder.CreateBr(after_block);

  Builder.SetInsertPoint(after_block);
  auto phi{Builder.CreatePHI(Builder.getInt1Ty(), 2)};
  phi->addIncoming(Builder.getTrue(), block);
  phi->addIncoming(result_, rhs_block);

  return need_bool_ ? phi : CastTo(phi, Builder.getInt32Ty(), false);
}

llvm::Value* CodeGen::LogicAndOp(const BinaryOpExpr& node) {
  auto block{Builder.GetInsertBlock()};
  auto parent_func{block->getParent()};
  auto rhs_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  node.lhs_->Accept(*this);
  result_ = CastToBool(result_);
  Builder.CreateCondBr(result_, rhs_block, after_block);

  Builder.SetInsertPoint(rhs_block);
  node.rhs_->Accept(*this);
  result_ = CastToBool(result_);
  Builder.CreateBr(after_block);

  Builder.SetInsertPoint(after_block);
  auto phi{Builder.CreatePHI(Builder.getInt1Ty(), 2)};
  phi->addIncoming(Builder.getFalse(), block);
  phi->addIncoming(result_, rhs_block);

  return need_bool_ ? phi : CastTo(phi, Builder.getInt32Ty(), false);
}

llvm::Value* CodeGen::AssignOp(const BinaryOpExpr& node) {
  auto lhs_ptr{GetPtr(*node.lhs_)};
  node.rhs_->Accept(*this);
  return Assign(lhs_ptr, result_);
}

llvm::Value* CodeGen::GetPtr(const AstNode& node) {
  auto p{dynamic_cast<const Object&>(node)};
  return p.local_ptr_;
}

llvm::Value* CodeGen::Assign(llvm::Value* lhs_ptr, llvm::Value* rhs) {
  return Builder.CreateAlignedStore(
      rhs, lhs_ptr, DataLayout.getABITypeAlignment(rhs->getType()));
}

void CodeGen::VisitForNoInc(const ForStmt& node) {
  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto cond_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto loop_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  Builder.CreateBr(cond_block);
  Builder.SetInsertPoint(cond_block);

  need_bool_ = true;
  node.cond_->Accept(*this);
  need_bool_ = false;
  result_ = CastToBool(result_);
  Builder.CreateCondBr(result_, loop_block, after_block);

  Builder.SetInsertPoint(loop_block);

  PushBlock(cond_block, after_block);
  node.block_->Accept(*this);
  PopBlock();

  if (!HasBrOrReturn()) {
    Builder.CreateBr(cond_block);
  }

  Builder.SetInsertPoint(after_block);
}

void CodeGen::VisitForNoCond(const ForStmt& node) {
  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto loop_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto inc_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  Builder.CreateBr(loop_block);
  Builder.SetInsertPoint(loop_block);

  Builder.SetInsertPoint(loop_block);
  // TODO
  PushBlock(nullptr, after_block);
  node.block_->Accept(*this);
  PopBlock();
  Builder.CreateBr(inc_block);

  Builder.SetInsertPoint(inc_block);

  node.inc_->Accept(*this);
  if (!HasBrOrReturn()) {
    Builder.CreateBr(loop_block);
  }

  Builder.SetInsertPoint(after_block);
}

void CodeGen::VisitForNoIncCond(const ForStmt& node) {
  auto parent_func{Builder.GetInsertBlock()->getParent()};
  auto loop_block{llvm::BasicBlock::Create(Context, "", parent_func)};
  auto after_block{llvm::BasicBlock::Create(Context, "", parent_func)};

  Builder.CreateBr(loop_block);

  // TODO
  Builder.SetInsertPoint(loop_block);

  PushBlock(nullptr, after_block);
  node.block_->Accept(*this);
  PopBlock();

  if (!HasBrOrReturn()) {
    Builder.CreateBr(loop_block);
  }

  Builder.SetInsertPoint(after_block);
}

void CodeGen::PushBlock(llvm::BasicBlock* continue_block,
                        llvm::BasicBlock* break_stack) {
  continue_stack_.push(continue_block);
  break_stack_.push(break_stack);
  has_br_or_return_.push({false, false});
}

void CodeGen::PopBlock() {
  continue_stack_.pop();
  break_stack_.pop();
  has_br_or_return_.pop();
}

bool CodeGen::HasBrOrReturn() const {
  if (std::empty(has_br_or_return_)) {
    return false;
  } else {
    auto [br, ret]{has_br_or_return_.top()};
    return br || ret;
  }
}

llvm::Value* CodeGen::NegOp(llvm::Value* value, bool is_unsigned) {
  if (IsIntegerTy(value)) {
    if (is_unsigned) {
      return Builder.CreateNeg(value);
    } else {
      return Builder.CreateNSWNeg(value);
    }
  } else if (IsFloatingPointTy(value)) {
    return Builder.CreateFNeg(value);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value* CodeGen::LogicNotOp(llvm::Value* value) {
  value = CastToBool(value);
  value = Builder.CreateXor(value, Builder.getTrue());

  return need_bool_ ? value : CastTo(value, Builder.getInt32Ty(), false);
}

std::string CodeGen::LLVMTypeToStr(llvm::Type* type) const {
  std::string s;
  llvm::raw_string_ostream rso{s};
  type->print(rso);
  return rso.str();
}

}  // namespace kcc
