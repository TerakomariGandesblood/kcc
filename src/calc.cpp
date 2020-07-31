//
// Created by kaiser on 2019/11/17.
//

#include "calc.h"

#include <cassert>
#include <stdexcept>

#include <llvm/Support/Casting.h>

#include "error.h"
#include "llvm_common.h"

namespace kcc {

CalcConstantExpr::CalcConstantExpr(const Location &loc) : loc_{loc} {}

llvm::Constant *CalcConstantExpr::Calc(const Expr *expr) {
  assert(expr != nullptr);

  try {
    expr->Accept(*this);
  } catch (const std::runtime_error &) {
    return nullptr;
  }

  return val_;
}

std::optional<std::int64_t> CalcConstantExpr::CalcInteger(const Expr *expr,
                                                          bool as_error) {
  auto val{Calc(expr)};

  if (!val) {
    if (as_error) {
      Error(expr->GetLoc(), "expect constant expression");
    } else {
      return {};
    }
  }

  if (auto p{llvm::dyn_cast<llvm::ConstantInt>(val)}) {
    return p->getValue().getSExtValue();
  } else {
    Error(expr->GetLoc(), "expect integer constant expression, but got '{}'",
          LLVMConstantToStr(val));
  }
}

llvm::Constant *CalcConstantExpr::Throw(llvm::Constant *value) {
  if (value == nullptr) {
    throw std::runtime_error{"expect constant expression"};
  } else {
    return value;
  }
}

void CalcConstantExpr::Visit(const UnaryOpExpr *node) {
  auto expr{node->GetExpr()};

  switch (node->GetOp()) {
  case Tag::kPlus:
    val_ = Throw(CalcConstantExpr{node->GetLoc()}.Calc(expr));
    break;
  case Tag::kMinus:
    val_ = NegOp(Throw(CalcConstantExpr{node->GetLoc()}.Calc(expr)),
                 expr->GetType()->IsUnsigned());
    break;
  case Tag::kTilde:
    val_ = llvm::ConstantExpr::getNot(
        Throw(CalcConstantExpr{node->GetLoc()}.Calc(expr)));
    break;
  case Tag::kExclaim:
    val_ = LogicNotOp(Throw(CalcConstantExpr{node->GetLoc()}.Calc(expr)));
    break;
  case Tag::kAmp:
    val_ = Addr(node);
    break;
  default:
    Throw();
  }
}

void CalcConstantExpr::Visit(const TypeCastExpr *node) {
  auto expr{node->GetExpr()};
  val_ = ConstantCastTo(Throw(CalcConstantExpr{node->GetLoc()}.Calc(expr)),
                        node->GetCastToType()->GetLLVMType(),
                        expr->GetType()->IsUnsigned());
}

void CalcConstantExpr::Visit(const BinaryOpExpr *node) {
#define L CalcConstantExpr{node->GetLoc()}.Calc(node->GetLHS())
#define R CalcConstantExpr{node->GetLoc()}.Calc(node->GetRHS())

  Throw(L);
  Throw(R);

  // 有时左右两边符号性可能不同
  // 如指针 + 整数(指针视为无符号）
  auto is_unsigned{node->GetLHS()->GetType()->IsUnsigned()};

  switch (node->GetOp()) {
  case Tag::kPlus:
    val_ = AddOp(L, R, is_unsigned);
    break;
  case Tag::kMinus:
    val_ = SubOp(L, R, is_unsigned);
    break;
  case Tag::kStar:
    val_ = MulOp(L, R, is_unsigned);
    break;
  case Tag::kSlash: {
    auto lhs{L};
    auto rhs{R};
    if (rhs->isZeroValue()) {
      Error(node->GetRHS(), "division by zero");
    }
    val_ = DivOp(lhs, rhs, is_unsigned);
    break;
  }
  case Tag::kPercent: {
    auto lhs{L};
    auto rhs{R};
    if (rhs->isZeroValue()) {
      Error(node->GetRHS(), "division by zero");
    }
    val_ = ModOp(lhs, rhs, is_unsigned);
    break;
  }
  case Tag::kAmp:
    val_ = AndOp(L, R);
    break;
  case Tag::kPipe:
    val_ = OrOp(L, R);
    break;
  case Tag::kCaret:
    val_ = XorOp(L, R);
    break;
  case Tag::kLessLess:
    val_ = ShlOp(L, R);
    break;
  case Tag::kGreaterGreater:
    val_ = ShrOp(L, R, is_unsigned);
    break;
  case Tag::kAmpAmp:
    val_ = LogicAndOp(node);
    break;
  case Tag::kPipePipe:
    val_ = LogicOrOp(node);
    break;
  case Tag::kEqualEqual:
    val_ = EqualOp(L, R);
    break;
  case Tag::kExclaimEqual:
    val_ = NotEqualOp(L, R);
    break;
  case Tag::kLess:
    val_ = LessOp(L, R, is_unsigned);
    break;
  case Tag::kGreater:
    val_ = GreaterOp(L, R, is_unsigned);
    break;
  case Tag::kLessEqual:
    val_ = LessEqualOp(L, R, is_unsigned);
    break;
  case Tag::kGreaterEqual:
    val_ = GreaterEqualOp(L, R, is_unsigned);
    break;
  default:
    Throw();
  }

#undef L
#undef R
#undef I_L
#undef I_R
}

void CalcConstantExpr::Visit(const ConditionOpExpr *node) {
  auto cond{Throw(CalcConstantExpr{node->GetLoc()}.Calc(node->GetCond()))};

  if (cond->isZeroValue()) {
    val_ = Throw(CalcConstantExpr{node->GetLoc()}.Calc(node->GetRHS()));
  } else {
    val_ = Throw(CalcConstantExpr{node->GetLoc()}.Calc(node->GetLHS()));
  }
}

void CalcConstantExpr::Visit(const ConstantExpr *node) {
  auto type{node->GetType()};

  if (type->IsIntegerTy()) {
    val_ = llvm::ConstantInt::get(type->GetLLVMType(), node->GetIntegerVal());
  } else if (type->IsFloatPointTy()) {
    val_ = llvm::ConstantFP::get(type->GetLLVMType(), node->GetFloatPointVal());
  } else {
    assert(false);
  }
}

void CalcConstantExpr::Visit(const EnumeratorExpr *node) {
  val_ = llvm::ConstantInt::get(node->GetType()->GetLLVMType(), node->GetVal());
}

void CalcConstantExpr::Visit(const StmtExpr *node) {
  if (!node->GetType()->IsVoidTy()) {
    auto last{node->GetBlock()->GetStmts().back()};
    assert(last->Kind() == AstNodeType::kExprStmt);

    val_ = Throw(CalcConstantExpr{node->GetLoc()}.Calc(
        dynamic_cast<ExprStmt *>(last)->GetExpr()));
  } else {
    Throw();
  }
}

void CalcConstantExpr::Visit(const StringLiteralExpr *node) {
  val_ = node->GetPtr();
}

void CalcConstantExpr::Visit(const FuncCallExpr *) { Throw(); }

void CalcConstantExpr::Visit(const IdentifierExpr *node) {
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

  val_ = func;
}

void CalcConstantExpr::Visit(const ObjectExpr *node) {
  auto type{node->GetType()};

  if ((node->IsGlobalVar() || node->IsLocalStaticVar()) &&
      (type->IsArrayTy() || type->IsStructOrUnionTy())) {
    val_ = node->GetGlobalPtr();
  } else {
    Throw();
  }
}

void CalcConstantExpr::Visit(const LabelStmt *) { assert(false); }

void CalcConstantExpr::Visit(const CaseStmt *) { assert(false); }

void CalcConstantExpr::Visit(const DefaultStmt *) { assert(false); }

void CalcConstantExpr::Visit(const CompoundStmt *) { assert(false); }

void CalcConstantExpr::Visit(const ExprStmt *) { assert(false); }

void CalcConstantExpr::Visit(const IfStmt *) { assert(false); }

void CalcConstantExpr::Visit(const SwitchStmt *) { assert(false); }

void CalcConstantExpr::Visit(const WhileStmt *) { assert(false); }

void CalcConstantExpr::Visit(const DoWhileStmt *) { assert(false); }

void CalcConstantExpr::Visit(const ForStmt *) { assert(false); }

void CalcConstantExpr::Visit(const GotoStmt *) { assert(false); }

void CalcConstantExpr::Visit(const ContinueStmt *) { assert(false); }

void CalcConstantExpr::Visit(const BreakStmt *) { assert(false); }

void CalcConstantExpr::Visit(const ReturnStmt *) { assert(false); }

void CalcConstantExpr::Visit(const TranslationUnit *) { assert(false); }

void CalcConstantExpr::Visit(const Declaration *) { assert(false); }

void CalcConstantExpr::Visit(const FuncDef *) { assert(false); }

llvm::Constant *CalcConstantExpr::NegOp(llvm::Constant *value,
                                        bool is_unsigned) {
  if (IsIntegerTy(value)) {
    if (is_unsigned) {
      return llvm::ConstantExpr::getNeg(value);
    } else {
      return llvm::ConstantExpr::getNSWNeg(value);
    }
  } else if (IsFloatingPointTy(value)) {
    return llvm::ConstantExpr::getFNeg(value);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant *CalcConstantExpr::LogicNotOp(llvm::Constant *value) {
  value = ConstantCastToBool(value);
  value =
      llvm::ConstantExpr::getXor(value, llvm::ConstantInt::getTrue(Context));

  return llvm::ConstantExpr::getZExt(value, Builder.getInt32Ty());
}

// 运算对象可以是:
// 静态存储期的变量
// 函数
// &a[0]
// a.b
llvm::Constant *CalcConstantExpr::Addr(const UnaryOpExpr *node) {
  auto expr{node->GetExpr()};
  assert(expr != nullptr);

  if (auto obj{dynamic_cast<const ObjectExpr *>(expr)}) {
    assert(obj->IsGlobalVar() || obj->IsLocalStaticVar());
    return obj->GetGlobalPtr();
    // Called C++ object pointer is null
  } else if (expr->Kind() == AstNodeType::kIdentifierExpr) {
    return Throw(CalcConstantExpr{node->GetLoc()}.Calc(expr));
  } else if (auto unary{dynamic_cast<const UnaryOpExpr *>(expr)}) {
    if (unary->GetOp() != Tag::kStar) {
      Throw();
    }

    auto binary{dynamic_cast<const BinaryOpExpr *>(unary->GetExpr())};

    if (!binary || binary->GetOp() != Tag::kPlus) {
      Throw();
    }

    assert(binary != nullptr);
    auto lhs{Throw(CalcConstantExpr{node->GetLoc()}.Calc(binary->GetLHS()))};
    auto rhs{Throw(CalcConstantExpr{node->GetLoc()}.Calc(binary->GetRHS()))};

    llvm::Constant *index[]{rhs};
    return llvm::ConstantExpr::getInBoundsGetElementPtr(nullptr, lhs, index);
  } else if (auto binary{dynamic_cast<const BinaryOpExpr *>(expr)}) {
    auto lhs{Throw(CalcConstantExpr{node->GetLoc()}.Calc(binary->GetLHS()))};

    auto member{dynamic_cast<const ObjectExpr *>(binary->GetRHS())};
    assert(member != nullptr);

    return llvm::ConstantExpr::getInBoundsGetElementPtr(
        nullptr, lhs, Builder.getInt64(member->GetIndexs().back().second));
  } else {
    Throw();
    return nullptr;
  }
}

llvm::Constant *CalcConstantExpr::AddOp(llvm::Constant *lhs,
                                        llvm::Constant *rhs, bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return llvm::ConstantExpr::getAdd(lhs, rhs);
    } else {
      return llvm::ConstantExpr::getNSWAdd(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return llvm::ConstantExpr::getFAdd(lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    return llvm::ConstantExpr::getInBoundsGetElementPtr(
        nullptr, lhs, llvm::ArrayRef<llvm::Constant *>{rhs});
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant *CalcConstantExpr::SubOp(llvm::Constant *lhs,
                                        llvm::Constant *rhs, bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return llvm::ConstantExpr::getSub(lhs, rhs);
    } else {
      return llvm::ConstantExpr::getNSWSub(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return llvm::ConstantExpr::getFSub(lhs, rhs);
  } else if (IsPointerTy(lhs) && IsIntegerTy(rhs)) {
    return llvm::ConstantExpr::getInBoundsGetElementPtr(
        nullptr, lhs,
        llvm::ArrayRef<llvm::Constant *>{NegOp(rhs, is_unsigned)});
  } else if (IsPointerTy(lhs) && IsPointerTy(rhs)) {
    auto type{lhs->getType()->getPointerElementType()};

    lhs = ConstantCastTo(lhs, Builder.getInt64Ty(), true);
    rhs = ConstantCastTo(rhs, Builder.getInt64Ty(), true);

    auto value{SubOp(lhs, rhs, true)};
    return DivOp(
        value, Builder.getInt64(Module->getDataLayout().getTypeAllocSize(type)),
        true);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant *CalcConstantExpr::MulOp(llvm::Constant *lhs,
                                        llvm::Constant *rhs, bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return llvm::ConstantExpr::getMul(lhs, rhs);
    } else {
      return llvm::ConstantExpr::getNSWMul(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return llvm::ConstantExpr::getFMul(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant *CalcConstantExpr::DivOp(llvm::Constant *lhs,
                                        llvm::Constant *rhs, bool is_unsigned) {
  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      return llvm::ConstantExpr::getUDiv(lhs, rhs);
    } else {
      return llvm::ConstantExpr::getSDiv(lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    return llvm::ConstantExpr::getFDiv(lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Constant *CalcConstantExpr::ModOp(llvm::Constant *lhs,
                                        llvm::Constant *rhs, bool is_unsigned) {
  if (is_unsigned) {
    return llvm::ConstantExpr::getURem(lhs, rhs);
  } else {
    return llvm::ConstantExpr::getSRem(lhs, rhs);
  }
}

llvm::Constant *CalcConstantExpr::OrOp(llvm::Constant *lhs,
                                       llvm::Constant *rhs) {
  return llvm::ConstantExpr::getOr(lhs, rhs);
}

llvm::Constant *CalcConstantExpr::AndOp(llvm::Constant *lhs,
                                        llvm::Constant *rhs) {
  return llvm::ConstantExpr::getAnd(lhs, rhs);
}

llvm::Constant *CalcConstantExpr::XorOp(llvm::Constant *lhs,
                                        llvm::Constant *rhs) {
  return llvm::ConstantExpr::getXor(lhs, rhs);
}

llvm::Constant *CalcConstantExpr::ShlOp(llvm::Constant *lhs,
                                        llvm::Constant *rhs) {
  return llvm::ConstantExpr::getShl(lhs, rhs);
}

llvm::Constant *CalcConstantExpr::ShrOp(llvm::Constant *lhs,
                                        llvm::Constant *rhs, bool is_unsigned) {
  if (is_unsigned) {
    return llvm::ConstantExpr::getLShr(lhs, rhs);
  } else {
    return llvm::ConstantExpr::getAShr(lhs, rhs);
  }
}

llvm::Constant *CalcConstantExpr::LessEqualOp(llvm::Constant *lhs,
                                              llvm::Constant *rhs,
                                              bool is_unsigned) {
  llvm::Constant *value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_ULE, lhs, rhs);
    } else {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SLE, lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = llvm::ConstantExpr::getFCmp(llvm::CmpInst::FCMP_OLE, lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_ULE, lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return llvm::ConstantExpr::getZExt(value, Builder.getInt32Ty());
}

llvm::Constant *CalcConstantExpr::LessOp(llvm::Constant *lhs,
                                         llvm::Constant *rhs,
                                         bool is_unsigned) {
  llvm::Constant *value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_ULT, lhs, rhs);
    } else {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SLT, lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = llvm::ConstantExpr::getFCmp(llvm::CmpInst::FCMP_OLT, lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_ULT, lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return llvm::ConstantExpr::getZExt(value, Builder.getInt32Ty());
}

llvm::Constant *CalcConstantExpr::GreaterEqualOp(llvm::Constant *lhs,
                                                 llvm::Constant *rhs,
                                                 bool is_unsigned) {
  llvm::Constant *value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_UGE, lhs, rhs);
    } else {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SGE, lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = llvm::ConstantExpr::getFCmp(llvm::CmpInst::FCMP_OGE, lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_UGE, lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return llvm::ConstantExpr::getZExt(value, Builder.getInt32Ty());
}

llvm::Constant *CalcConstantExpr::GreaterOp(llvm::Constant *lhs,
                                            llvm::Constant *rhs,
                                            bool is_unsigned) {
  llvm::Constant *value{};

  if (IsIntegerTy(lhs)) {
    if (is_unsigned) {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_UGT, lhs, rhs);
    } else {
      value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_SGT, lhs, rhs);
    }
  } else if (IsFloatingPointTy(lhs)) {
    value = llvm::ConstantExpr::getFCmp(llvm::CmpInst::FCMP_OGT, lhs, rhs);
  } else if (IsPointerTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_UGT, lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return llvm::ConstantExpr::getZExt(value, Builder.getInt32Ty());
}

llvm::Constant *CalcConstantExpr::EqualOp(llvm::Constant *lhs,
                                          llvm::Constant *rhs) {
  llvm::Constant *value{};

  if (IsIntegerTy(lhs) || IsPointerTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_EQ, lhs, rhs);
  } else if (IsFloatingPointTy(lhs)) {
    value = llvm::ConstantExpr::getFCmp(llvm::CmpInst::FCMP_OEQ, lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return llvm::ConstantExpr::getZExt(value, Builder.getInt32Ty());
}

llvm::Constant *CalcConstantExpr::NotEqualOp(llvm::Constant *lhs,
                                             llvm::Constant *rhs) {
  llvm::Constant *value{};

  if (IsIntegerTy(lhs) || IsPointerTy(lhs)) {
    value = llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_NE, lhs, rhs);
  } else if (IsFloatingPointTy(lhs)) {
    value = llvm::ConstantExpr::getFCmp(llvm::CmpInst::FCMP_ONE, lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }

  return llvm::ConstantExpr::getZExt(value, Builder.getInt32Ty());
}

llvm::Constant *CalcConstantExpr::LogicOrOp(const BinaryOpExpr *node) {
  auto lhs{Throw(CalcConstantExpr{}.Calc(node->GetLHS()))};

  if (lhs->isZeroValue()) {
    auto rhs{Throw(CalcConstantExpr{}.Calc(node->GetRHS()))};
    return llvm::ConstantInt::get(Builder.getInt32Ty(), !rhs->isZeroValue());
  } else {
    return llvm::ConstantInt::get(Builder.getInt32Ty(), 1);
  }
}

llvm::Constant *CalcConstantExpr::LogicAndOp(const BinaryOpExpr *node) {
  auto lhs{Throw(CalcConstantExpr{}.Calc(node->GetLHS()))};

  if (lhs->isZeroValue()) {
    return llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
  } else {
    auto rhs{Throw(CalcConstantExpr{}.Calc(node->GetRHS()))};
    return llvm::ConstantInt::get(Builder.getInt32Ty(), !rhs->isZeroValue());
  }
}

} // namespace kcc
