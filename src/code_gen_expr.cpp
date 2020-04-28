//
// Created by kaiser on 2019/12/13.
//

#include "code_gen.h"

#include <assert.h>
#include <algorithm>
#include <vector>

#include <llvm/IR/CFG.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/Casting.h>

#include "calc.h"
#include "llvm_common.h"

namespace kcc {

void CodeGen::Visit(const UnaryOpExpr* node) {
  auto is_unsigned{node->GetExpr()->GetType()->IsUnsigned()};

  switch (node->GetOp()) {
    case Tag::kPlusPlus:
      result_ = IncOrDec(node->GetExpr(), true, false);
      break;
    case Tag::kPostfixPlusPlus:
      result_ = IncOrDec(node->GetExpr(), true, true);
      break;
    case Tag::kMinusMinus:
      result_ = IncOrDec(node->GetExpr(), false, false);
      break;
    case Tag::kPostfixMinusMinus:
      result_ = IncOrDec(node->GetExpr(), false, true);
      break;
    case Tag::kPlus:
      node->GetExpr()->Accept(*this);
      break;
    case Tag::kMinus:
      node->GetExpr()->Accept(*this);
      TryEmitLocation(node);
      result_ = NegOp(result_, is_unsigned);
      break;
    case Tag::kTilde:
      node->GetExpr()->Accept(*this);
      TryEmitLocation(node);
      result_ = Builder.CreateNot(result_);
      break;
    case Tag::kExclaim:
      node->GetExpr()->Accept(*this);
      TryEmitLocation(node);
      result_ = LogicNotOp(result_);
      break;
    case Tag::kStar:
      result_ = Deref(node);
      break;
    case Tag::kAmp:
      result_ = GetPtr(node->GetExpr());
      break;
    default:
      assert(false);
  }
}

void CodeGen::Visit(const TypeCastExpr* node) {
  node->GetExpr()->Accept(*this);
  TryEmitLocation(node);
  result_ = CastTo(result_, node->GetCastToType()->GetLLVMType(),
                   node->GetExpr()->GetType()->IsUnsigned());
}

void CodeGen::Visit(const BinaryOpExpr* node) {
  switch (node->GetOp()) {
    case Tag::kPipePipe:
      result_ = LogicOrOp(node);
      return;
    case Tag::kAmpAmp:
      result_ = LogicAndOp(node);
      return;
    case Tag::kEqual:
      result_ = AssignOp(node);
      return;
    case Tag::kPeriod:
      result_ = MemberRef(node);
      return;
    default:
      break;
  }

  node->GetLHS()->Accept(*this);
  auto lhs{result_};
  node->GetRHS()->Accept(*this);
  auto rhs{result_};

  TryEmitLocation(node);

  bool is_unsigned{node->GetLHS()->GetType()->IsUnsigned()};

  switch (node->GetOp()) {
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

void CodeGen::Visit(const ConditionOpExpr* node) {
  if (auto cond{CalcConstantExpr{}.Calc(node->GetCond())}) {
    TryEmitLocation(node);
    auto live{node->GetLHS()}, dead{node->GetRHS()};
    if (cond->isZeroValue()) {
      std::swap(live, dead);
    }

    live->Accept(*this);
    return;
  }

  // x ? 4 : 5
  if (IsCheapEnoughToEvaluateUnconditionally(node->GetLHS()) &&
      IsCheapEnoughToEvaluateUnconditionally(node->GetRHS())) {
    auto cond{EvaluateExprAsBool(node->GetCond())};
    node->GetLHS()->Accept(*this);
    auto lhs{result_};
    node->GetRHS()->Accept(*this);
    TryEmitLocation(node);
    result_ = Builder.CreateSelect(cond, lhs, result_);
    return;
  }

  auto lhs_block{CreateBasicBlock("cond.true")};
  auto rhs_block{CreateBasicBlock("cond.false")};
  auto end_block{CreateBasicBlock("cond.end")};

  EmitBranchOnBoolExpr(node->GetCond(), lhs_block, rhs_block);

  EmitBlock(lhs_block);
  node->GetLHS()->Accept(*this);
  auto lhs{result_};
  lhs_block = Builder.GetInsertBlock();
  EmitBranch(end_block);

  EmitBlock(rhs_block);
  node->GetRHS()->Accept(*this);
  auto rhs{result_};
  rhs_block = Builder.GetInsertBlock();
  EmitBranch(end_block);

  EmitBlock(end_block);

  if (lhs->getType()->isVoidTy() || rhs->getType()->isVoidTy()) {
    return;
  }

  assert(lhs->getType() == rhs->getType());

  TryEmitLocation(node);

  auto phi{Builder.CreatePHI(lhs->getType(), 2)};
  phi->addIncoming(lhs, lhs_block);
  phi->addIncoming(rhs, rhs_block);

  result_ = phi;
}

// LLVM 默认使用本机 C 调用约定
void CodeGen::Visit(const FuncCallExpr* node) {
  if (MayCallBuiltinFunc(node)) {
    return;
  }

  node->GetCallee()->Accept(*this);
  auto callee{result_};

  std::vector<llvm::Value*> args;
  Load_Struct_Obj();
  for (const auto& item : node->GetArgs()) {
    item->Accept(*this);
    args.push_back(result_);
  }
  Finish_Load();

  TryEmitLocation(node);
  result_ = Builder.CreateCall(callee, args);
}

// 常量用 ConstantFP / ConstantInt 类表示
// 在 LLVM IR 中, 常量都是唯一且共享的
void CodeGen::Visit(const ConstantExpr* node) {
  TryEmitLocation(node);

  auto type{node->GetType()->GetLLVMType()};

  if (type->isIntegerTy()) {
    result_ = llvm::ConstantInt::get(Context, node->GetIntegerVal());
  } else if (type->isFloatingPointTy()) {
    result_ = llvm::ConstantFP::get(type, node->GetFloatPointVal());
  } else {
    assert(false);
  }
}

// 注意已经添加空字符了
void CodeGen::Visit(const StringLiteralExpr* node) {
  TryEmitLocation(node);
  result_ = node->GetPtr();
}

void CodeGen::Visit(const IdentifierExpr* node) {
  TryEmitLocation(node);

  auto type{node->GetType()};
  assert(type->IsFunctionTy());

  auto name{node->GetName()};

  auto func{Module->getFunction(name)};
  if (!func) {
    func = llvm::Function::Create(
        llvm::cast<llvm::FunctionType>(type->GetLLVMType()),
        node->GetLinkage() == Linkage::kInternal
            ? llvm::Function::InternalLinkage
            : llvm::Function::ExternalLinkage,
        name, Module.get());
  }

  result_ = func;
}

void CodeGen::Visit(const EnumeratorExpr* node) {
  TryEmitLocation(node);
  result_ = llvm::ConstantInt::get(Builder.getInt32Ty(), node->GetVal());
}

void CodeGen::Visit(const ObjectExpr* node) {
  llvm::Value* ptr;
  if (node->IsGlobalVar() || node->IsLocalStaticVar()) {
    ptr = node->GetGlobalPtr();
  } else {
    ptr = node->GetLocalPtr();
  }

  is_volatile_ = node->GetQualType().IsVolatile();

  auto type{node->GetType()};
  if (type->IsArrayTy() || (type->IsStructOrUnionTy() && !load_struct_)) {
    result_ = ptr;
  } else {
    result_ = Builder.CreateLoad(ptr, is_volatile_);
    is_volatile_ = false;
  }
}

void CodeGen::Visit(const StmtExpr* node) {
  TryEmitLocation(node);
  node->GetBlock()->Accept(*this);
}

llvm::Value* CodeGen::IncOrDec(const Expr* expr, bool is_inc, bool is_postfix) {
  auto is_unsigned{expr->GetType()->IsUnsigned()};
  auto lhs_ptr{GetPtr(expr)};

  TryEmitLocation(expr);
  llvm::Value* lhs_value{Builder.CreateLoad(lhs_ptr, is_volatile_)};

  if (is_bit_field_) {
    auto size{bit_field_->GetType()->IsCharacterTy() ? 8 : 32};

    lhs_value = GetBitFieldValue(
        lhs_value, size, bit_field_->GetBitFieldWidth(),
        bit_field_->GetBitFieldBegin(), bit_field_->GetType()->IsUnsigned());
  }

  llvm::Value* rhs_value{};

  llvm::Value* one_value{};
  auto type{expr->GetType()->GetLLVMType()};

  if (type->isIntegerTy()) {
    one_value = llvm::ConstantInt::get(type, 1);
  } else if (type->isFloatingPointTy()) {
    one_value = llvm::ConstantFP::get(type, 1.0);
  } else if (type->isPointerTy()) {
    one_value = Builder.getInt64(1);
  } else {
    assert(false);
  }

  if (is_inc) {
    rhs_value = AddOp(lhs_value, one_value, is_unsigned);
  } else {
    rhs_value = AddOp(lhs_value, NegOp(one_value, false), is_unsigned);
  }

  Assign(lhs_ptr, rhs_value, is_unsigned);

  return is_postfix ? lhs_value : rhs_value;
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
  result_ = Builder.CreateNot(CastToBool(value));
  return Builder.CreateZExt(result_, Builder.getInt32Ty());
}

llvm::Value* CodeGen::Deref(const UnaryOpExpr* node) {
  auto binary{dynamic_cast<const BinaryOpExpr*>(node->GetExpr())};
  // e.g. a[1] / *(p + 1)
  if (binary && binary->GetOp() == Tag::kPlus) {
    binary->GetLHS()->Accept(*this);
    auto lhs{result_};
    binary->GetRHS()->Accept(*this);
    TryEmitLocation(node);

    if (IsArrayPointer(lhs->getType())) {
      result_ = Builder.CreateInBoundsGEP(lhs, {result_, Builder.getInt64(0)});
    } else {
      result_ = Builder.CreateInBoundsGEP(lhs, {result_});
      result_ = Builder.CreateLoad(result_, is_volatile_);
      is_volatile_ = false;
    }
  } else if (IsFuncPointer(node->GetExpr()->GetType()->GetLLVMType())) {
    TryEmitLocation(node);
    node->GetExpr()->Accept(*this);
  } else {
    node->GetExpr()->Accept(*this);
    TryEmitLocation(node);
    if (!node->GetType()->IsArrayTy()) {
      result_ = Builder.CreateLoad(result_, is_volatile_);
    }
    is_volatile_ = false;
  }

  return result_;
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
    return Builder.CreateInBoundsGEP(lhs, {rhs});
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
    return Builder.CreateInBoundsGEP(lhs, {Builder.CreateNeg(rhs)});
  } else if (IsPointerTy(lhs) && IsPointerTy(rhs)) {
    auto type{lhs->getType()->getPointerElementType()};

    lhs = CastTo(lhs, Builder.getInt64Ty(), true);
    rhs = CastTo(rhs, Builder.getInt64Ty(), true);

    auto value{SubOp(lhs, rhs, true)};
    return DivOp(
        value, Builder.getInt64(Module->getDataLayout().getTypeAllocSize(type)),
        true);
  } else {
    assert(false);
    return nullptr;
  }
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
  llvm::Value* value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = Builder.CreateICmpULE(lhs, rhs);
    } else {
      value = Builder.CreateICmpSLE(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = Builder.CreateFCmpOLE(lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = Builder.CreateICmpULE(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return Builder.CreateZExt(value, Builder.getInt32Ty());
}

llvm::Value* CodeGen::LessOp(llvm::Value* lhs, llvm::Value* rhs,
                             bool is_unsigned) {
  llvm::Value* value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = Builder.CreateICmpULT(lhs, rhs);
    } else {
      value = Builder.CreateICmpSLT(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = Builder.CreateFCmpOLT(lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = Builder.CreateICmpULT(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return Builder.CreateZExt(value, Builder.getInt32Ty());
}

llvm::Value* CodeGen::GreaterEqualOp(llvm::Value* lhs, llvm::Value* rhs,
                                     bool is_unsigned) {
  llvm::Value* value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = Builder.CreateICmpUGE(lhs, rhs);
    } else {
      value = Builder.CreateICmpSGE(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = Builder.CreateFCmpOGE(lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = Builder.CreateICmpUGE(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return Builder.CreateZExt(value, Builder.getInt32Ty());
}

llvm::Value* CodeGen::GreaterOp(llvm::Value* lhs, llvm::Value* rhs,
                                bool is_unsigned) {
  llvm::Value* value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = Builder.CreateICmpUGT(lhs, rhs);
    } else {
      value = Builder.CreateICmpSGT(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = Builder.CreateFCmpOGT(lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = Builder.CreateICmpUGT(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return Builder.CreateZExt(value, Builder.getInt32Ty());
}

llvm::Value* CodeGen::EqualOp(llvm::Value* lhs, llvm::Value* rhs) {
  llvm::Value* value{};

  if (IsIntegerTy(lhs) || IsPointerTy(lhs)) {
    value = Builder.CreateICmpEQ(lhs, rhs);
  } else if (IsFloatingPointTy(lhs)) {
    value = Builder.CreateFCmpOEQ(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return Builder.CreateZExt(value, Builder.getInt32Ty());
}

llvm::Value* CodeGen::NotEqualOp(llvm::Value* lhs, llvm::Value* rhs) {
  llvm::Value* value{};

  if (IsIntegerTy(lhs) || IsPointerTy(lhs)) {
    value = Builder.CreateICmpNE(lhs, rhs);
  } else if (IsFloatingPointTy(lhs)) {
    value = Builder.CreateFCmpONE(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return Builder.CreateZExt(value, Builder.getInt32Ty());
}

llvm::Value* CodeGen::LogicOrOp(const BinaryOpExpr* node) {
  if (auto lhs{CalcConstantExpr{}.Calc(node->GetLHS())}) {
    if (lhs->isZeroValue()) {
      auto rhs{EvaluateExprAsBool(node->GetRHS())};
      return Builder.CreateZExt(rhs, Builder.getInt32Ty());
    } else {
      return Builder.getInt32(1);
    }
  }

  auto rhs_block{CreateBasicBlock("logic.or.rhs")};
  auto end_block{CreateBasicBlock("logic.or.end")};

  EmitBranchOnBoolExpr(node->GetLHS(), end_block, rhs_block);
  auto phi{llvm::PHINode::Create(Builder.getInt1Ty(), 2, "", end_block)};

  // llvm::predecessors 获取 basic block 的所有前驱
  for (const auto& item : llvm::predecessors(end_block)) {
    phi->addIncoming(Builder.getTrue(), item);
  }

  EmitBlock(rhs_block);
  auto rhs_value{EvaluateExprAsBool(node->GetRHS())};

  rhs_block = Builder.GetInsertBlock();
  EmitBlock(end_block);

  TryEmitLocation(node);
  phi->addIncoming(rhs_value, rhs_block);

  return Builder.CreateZExt(phi, Builder.getInt32Ty());
}

llvm::Value* CodeGen::LogicAndOp(const BinaryOpExpr* node) {
  if (auto lhs{CalcConstantExpr{}.Calc(node->GetLHS())}) {
    if (lhs->isOneValue()) {
      auto rhs{EvaluateExprAsBool(node->GetRHS())};
      return Builder.CreateZExt(rhs, Builder.getInt32Ty());
    } else {
      return Builder.getInt32(0);
    }
  }

  auto rhs_block{CreateBasicBlock("logic.and.rhs")};
  auto end_block{CreateBasicBlock("logic.and.end")};

  EmitBranchOnBoolExpr(node->GetLHS(), rhs_block, end_block);
  auto phi{llvm::PHINode::Create(Builder.getInt1Ty(), 2, "", end_block)};

  for (const auto& item : llvm::predecessors(end_block)) {
    phi->addIncoming(Builder.getFalse(), item);
  }

  EmitBlock(rhs_block);
  auto rhs_value{EvaluateExprAsBool(node->GetRHS())};

  rhs_block = Builder.GetInsertBlock();

  EmitBlock(end_block);

  TryEmitLocation(node);
  phi->addIncoming(rhs_value, rhs_block);

  return Builder.CreateZExt(phi, Builder.getInt32Ty());
}

llvm::Value* CodeGen::AssignOp(const BinaryOpExpr* node) {
  Load_Struct_Obj();
  node->GetRHS()->Accept(*this);
  Finish_Load();
  auto rhs{result_};

  auto lhs_ptr{GetPtr(node->GetLHS())};
  TryEmitLocation(node);

  return Assign(lhs_ptr, rhs, node->GetRHS()->GetType()->IsUnsigned());
}

llvm::Value* CodeGen::MemberRef(const BinaryOpExpr* node) {
  auto ptr{GetPtr(node)};
  TryEmitLocation(node);
  auto type{ptr->getType()->getPointerElementType()};

  if (is_bit_field_) {
    result_ = Builder.CreateLoad(ptr, is_volatile_);

    auto size{bit_field_->GetType()->IsCharacterTy() ? 8 : 32};

    result_ = GetBitFieldValue(result_, size, bit_field_->GetBitFieldWidth(),
                               bit_field_->GetBitFieldBegin(),
                               bit_field_->GetType()->IsUnsigned());

    is_bit_field_ = false;
    bit_field_ = nullptr;
  } else {
    if (type->isArrayTy() || (type->isStructTy() && !load_struct_)) {
      result_ = ptr;
    } else {
      result_ = Builder.CreateLoad(ptr, is_volatile_);
    }
  }

  return result_;
}

llvm::Value* CodeGen::Assign(llvm::Value* lhs_ptr, llvm::Value* rhs,
                             bool is_unsigned) {
  if (is_bit_field_) {
    result_ = Builder.CreateLoad(lhs_ptr, is_volatile_);

    auto size{bit_field_->GetType()->IsCharacterTy() ? 8 : 32};
    result_ = GetBitField(result_, size, bit_field_->GetBitFieldWidth(),
                          bit_field_->GetBitFieldBegin());

    rhs = Builder.CreateShl(rhs, bit_field_->GetBitFieldBegin());
    rhs = CastTo(rhs, Builder.getInt32Ty(), is_unsigned);
    result_ = Builder.CreateOr(result_, rhs);

    Builder.CreateStore(result_, lhs_ptr, is_volatile_);

    if (!TestAndClearIgnoreAssignResult()) {
      result_ = Builder.CreateLoad(lhs_ptr, is_volatile_);

      result_ = GetBitFieldValue(result_, size, bit_field_->GetBitFieldWidth(),
                                 bit_field_->GetBitFieldBegin(),
                                 bit_field_->GetType()->IsUnsigned());

      is_bit_field_ = false;
      bit_field_ = nullptr;
      is_volatile_ = false;

      return result_;
    } else {
      is_bit_field_ = false;
      bit_field_ = nullptr;
      is_volatile_ = false;

      return lhs_ptr;
    }
  } else {
    Builder.CreateStore(rhs, lhs_ptr, is_volatile_);

    if (!TestAndClearIgnoreAssignResult()) {
      result_ = Builder.CreateLoad(lhs_ptr, is_volatile_);
      is_volatile_ = false;
      return result_;
    } else {
      is_volatile_ = false;
      return lhs_ptr;
    }
  }
}

bool CodeGen::MayCallBuiltinFunc(const FuncCallExpr* node) {
  auto func_name{node->GetFuncType()->FuncGetName()};

  if (func_name == "__builtin_va_start") {
    result_ = VaStart(node->GetArgs().front());
    return true;
  } else if (func_name == "__builtin_va_end") {
    result_ = VaEnd(node->GetArgs().front());
    return true;
  } else if (func_name == "__builtin_va_arg_sub") {
    assert(node->GetVaArgType() != nullptr);
    result_ =
        VaArg(node->GetArgs().front(), node->GetVaArgType()->GetLLVMType());
    return true;
  } else if (func_name == "__builtin_va_copy") {
    result_ = VaCopy(node->GetArgs()[0], node->GetArgs()[1]);
    return true;
  } else if (func_name == "__sync_synchronize") {
    result_ = SyncSynchronize();
    return true;
  } else if (func_name == "__builtin_alloca") {
    result_ = Alloc(node->GetArgs().front());
    return true;
  } else if (func_name == "__builtin_popcount") {
    result_ = PopCount(node->GetArgs().front());
    return true;
  } else if (func_name == "__builtin_clz") {
    result_ = Clz(node->GetArgs().front());
    return true;
  } else if (func_name == "__builtin_ctz") {
    result_ = Ctz(node->GetArgs().front());
    return true;
  } else if (func_name == "__builtin_expect") {
    node->GetArgs().front()->Accept(*this);
    return true;
  } else if (func_name == "__builtin_isinf_sign") {
    result_ = IsInfSign(node->GetArgs().front());
    return true;
  } else if (func_name == "__builtin_isfinite") {
    result_ = IsFinite(node->GetArgs().front());
    return true;
  } else if (func_name == "__builtin_bswap16") {
    result_ = Bswap16(node->GetArgs().front());
    return true;
  } else if (func_name == "__builtin_bswap32") {
    result_ = Bswap32(node->GetArgs().front());
    return true;
  } else if (func_name == "__builtin_bswap64") {
    result_ = Bswap64(node->GetArgs().front());
    return true;
  } else {
    return false;
  }
}

llvm::Value* CodeGen::VaStart(Expr* arg) {
  static auto func_type{llvm::FunctionType::get(
      Builder.getVoidTy(), {Builder.getInt8PtrTy()}, false)};

  static auto va_start{llvm::Function::Create(func_type,
                                              llvm::Function::ExternalLinkage,
                                              "llvm.va_start", Module.get())};

  arg->Accept(*this);

  result_ = Builder.CreateBitCast(result_, Builder.getInt8PtrTy());
  return Builder.CreateCall(va_start, {result_});
}

llvm::Value* CodeGen::VaEnd(Expr* arg) {
  static auto func_type{llvm::FunctionType::get(
      Builder.getVoidTy(), {Builder.getInt8PtrTy()}, false)};

  static auto va_end{llvm::Function::Create(
      func_type, llvm::Function::ExternalLinkage, "llvm.va_end", Module.get())};

  arg->Accept(*this);

  result_ = Builder.CreateBitCast(result_, Builder.getInt8PtrTy());
  return Builder.CreateCall(va_end, {result_});
}

llvm::Value* CodeGen::VaArg(Expr* arg, llvm::Type* type) {
  auto lhs_block{CreateBasicBlock("va.arg.lhs")};
  auto rhs_block{CreateBasicBlock("va.arg.rhs")};
  auto end_block{CreateBasicBlock("va.arg.end")};

  llvm::Value* offset_ptr{};
  llvm::Value* offset{};

  arg->Accept(*this);
  auto ptr{result_};

  if (type->isIntegerTy() || type->isPointerTy()) {
    offset_ptr = Builder.CreateStructGEP(ptr, 0);
    offset = Builder.CreateLoad(offset_ptr);
    result_ = Builder.CreateICmpULE(
        offset, llvm::ConstantInt::get(Builder.getInt32Ty(), 40));
  } else if (type->isFloatingPointTy()) {
    offset_ptr = Builder.CreateStructGEP(ptr, 1);
    offset = Builder.CreateLoad(offset_ptr);
    result_ = Builder.CreateICmpULE(
        offset, llvm::ConstantInt::get(Builder.getInt32Ty(), 160));
  } else {
    assert(false);
  }

  Builder.CreateCondBr(result_, lhs_block, rhs_block);

  EmitBlock(lhs_block);
  result_ = Builder.CreateStructGEP(ptr, 3);
  result_ = Builder.CreateLoad(result_);
  result_ = Builder.CreateGEP(result_, offset);
  auto result_ptr{Builder.CreateBitCast(result_, type->getPointerTo())};

  if (type->isIntegerTy() || type->isPointerTy()) {
    result_ = Builder.CreateAdd(
        offset, llvm::ConstantInt::get(Builder.getInt32Ty(), 8));
  } else if (type->isFloatingPointTy()) {
    result_ = Builder.CreateAdd(
        offset, llvm::ConstantInt::get(Builder.getInt32Ty(), 16));
  } else {
    assert(false);
  }

  Builder.CreateStore(result_, offset_ptr);
  EmitBranch(end_block);

  EmitBlock(rhs_block);
  auto pp{Builder.CreateStructGEP(ptr, 2)};
  result_ = Builder.CreateLoad(pp);
  auto result_ptr2{Builder.CreateBitCast(result_, type->getPointerTo())};
  result_ = Builder.CreateGEP(result_,
                              llvm::ConstantInt::get(Builder.getInt32Ty(), 8));
  Builder.CreateStore(result_, pp);
  EmitBranch(end_block);

  EmitBlock(end_block);
  auto phi{Builder.CreatePHI(type->getPointerTo(), 2)};
  phi->addIncoming(result_ptr, lhs_block);
  phi->addIncoming(result_ptr2, rhs_block);

  return phi;
}

llvm::Value* CodeGen::VaCopy(Expr* arg, Expr* arg2) {
  static auto func_type{llvm::FunctionType::get(
      Builder.getVoidTy(), {Builder.getInt8PtrTy(), Builder.getInt8PtrTy()},
      false)};

  static auto va_copy{llvm::Function::Create(func_type,
                                             llvm::Function::ExternalLinkage,
                                             "llvm.va_copy", Module.get())};

  arg->Accept(*this);
  auto param{result_};
  arg2->Accept(*this);
  auto param2{result_};

  return Builder.CreateCall(
      va_copy, {Builder.CreateBitCast(param, Builder.getInt8PtrTy()),
                Builder.CreateBitCast(param2, Builder.getInt8PtrTy())});
}

llvm::Value* CodeGen::SyncSynchronize() {
  return Builder.CreateFence(llvm::AtomicOrdering::SequentiallyConsistent,
                             llvm::SyncScope::System);
}

llvm::Value* CodeGen::Alloc(Expr* arg) {
  arg->Accept(*this);
  return Builder.CreateAlloca(Builder.getInt8Ty(), result_);
}

llvm::Value* CodeGen::PopCount(Expr* arg) {
  static auto func_type{llvm::FunctionType::get(Builder.getInt32Ty(),
                                                {Builder.getInt32Ty()}, false)};

  static auto ctpop_i32{llvm::Function::Create(func_type,
                                               llvm::Function::ExternalLinkage,
                                               "llvm.ctpop.i32", Module.get())};

  arg->Accept(*this);
  return Builder.CreateCall(ctpop_i32, {result_});
}

llvm::Value* CodeGen::Clz(Expr* arg) {
  static auto func_type{llvm::FunctionType::get(
      Builder.getInt32Ty(), {Builder.getInt32Ty(), Builder.getInt1Ty()},
      false)};

  static auto ctlz_i32{llvm::Function::Create(func_type,
                                              llvm::Function::ExternalLinkage,
                                              "llvm.ctlz.i32", Module.get())};

  arg->Accept(*this);
  return Builder.CreateCall(ctlz_i32, {result_, Builder.getTrue()});
}

llvm::Value* CodeGen::Ctz(Expr* arg) {
  static auto func_type{llvm::FunctionType::get(
      Builder.getInt32Ty(), {Builder.getInt32Ty(), Builder.getInt1Ty()},
      false)};

  static auto cttz_i32{llvm::Function::Create(func_type,
                                              llvm::Function::ExternalLinkage,
                                              "llvm.cttz.i32", Module.get())};

  arg->Accept(*this);
  return Builder.CreateCall(cttz_i32, {result_, Builder.getTrue()});
}

llvm::Value* CodeGen::IsInfSign(Expr* arg) {
  static auto func_type{llvm::FunctionType::get(Builder.getFloatTy(),
                                                {Builder.getFloatTy()}, false)};

  static auto fabs_f32{llvm::Function::Create(func_type,
                                              llvm::Function::ExternalLinkage,
                                              "llvm.fabs.f32", Module.get())};

  arg->Accept(*this);
  auto load{result_};
  result_ = Builder.CreateCall(fabs_f32, {result_});

  auto mark{Builder.CreateFCmpOEQ(
      result_,
      llvm::ConstantFP::get(
          Builder.getFloatTy(),
          llvm::APFloat::getInf(GetFloatTypeSemantics(Builder.getFloatTy()))))};

  result_ = Builder.CreateBitCast(load, Builder.getInt32Ty());
  result_ = Builder.CreateICmpSLT(result_, Builder.getInt32(0));
  result_ =
      Builder.CreateSelect(result_, Builder.getInt32(-1), Builder.getInt32(1));

  return Builder.CreateSelect(mark, result_, Builder.getInt32(0));
}

llvm::Value* CodeGen::IsFinite(Expr* arg) {
  static auto func_type{llvm::FunctionType::get(Builder.getFloatTy(),
                                                {Builder.getFloatTy()}, false)};

  static auto fabs_f32{llvm::Function::Create(func_type,
                                              llvm::Function::ExternalLinkage,
                                              "llvm.fabs.f32", Module.get())};

  arg->Accept(*this);
  result_ = Builder.CreateCall(fabs_f32, {result_});

  result_ = Builder.CreateFCmpONE(
      result_,
      llvm::ConstantFP::get(
          Builder.getFloatTy(),
          llvm::APFloat::getInf(GetFloatTypeSemantics(Builder.getFloatTy()))));

  return Builder.CreateZExt(result_, Builder.getInt32Ty());
}

llvm::Value* CodeGen::Bswap16(Expr* arg) {
  static auto func_type{llvm::FunctionType::get(Builder.getInt16Ty(),
                                                {Builder.getInt16Ty()}, false)};

  static auto bswap_i16{llvm::Function::Create(func_type,
                                               llvm::Function::ExternalLinkage,
                                               "llvm.bswap.i16", Module.get())};

  arg->Accept(*this);
  return Builder.CreateCall(bswap_i16, {result_});
}

llvm::Value* CodeGen::Bswap32(Expr* arg) {
  static auto func_type{llvm::FunctionType::get(Builder.getInt32Ty(),
                                                {Builder.getInt32Ty()}, false)};

  static auto bswap_i32{llvm::Function::Create(func_type,
                                               llvm::Function::ExternalLinkage,
                                               "llvm.bswap.i32", Module.get())};

  arg->Accept(*this);
  return Builder.CreateCall(bswap_i32, {result_});
}

llvm::Value* CodeGen::Bswap64(Expr* arg) {
  static auto func_type{llvm::FunctionType::get(Builder.getInt64Ty(),
                                                {Builder.getInt64Ty()}, false)};

  static auto bswap_i64{llvm::Function::Create(func_type,
                                               llvm::Function::ExternalLinkage,
                                               "llvm.bswap.i64", Module.get())};

  arg->Accept(*this);
  return Builder.CreateCall(bswap_i64, {result_});
}

}  // namespace kcc
