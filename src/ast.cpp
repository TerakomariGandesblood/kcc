//
// Created by kaiser on 2019/10/31.
//

#include "ast.h"

#include <algorithm>

#include <magic_enum.hpp>

#include "error.h"
#include "llvm_common.h"
#include "memory_pool.h"
#include "visitor.h"

namespace kcc {

/*
 * AstNode
 */
std::string AstNode::KindQString() const {
  std::string str{magic_enum::enum_name(Kind())};
  return str;
}

const Location &AstNode::GetLoc() const { return loc_; }

void AstNode::SetLoc(const Location &loc) { loc_ = loc; }

/*
 * Expr
 */
QualType Expr::GetQualType() const { return type_; }

Type *Expr::GetType() { return type_.GetType(); }

const Type *Expr::GetType() const { return type_.GetType(); }

bool Expr::IsConst() const { return type_.IsConst(); }

bool Expr::IsVolatile() const { return type_.IsVolatile(); }

void Expr::EnsureCompatible(QualType lhs, QualType rhs) const {
  if (!lhs->Compatible(rhs.GetType())) {
    Error(loc_, "incompatible types: '{}' vs '{}'", lhs.ToString(),
          rhs.ToString());
  }
}

void Expr::EnsureCompatibleConvertVoidPtr(Expr *&lhs, Expr *&rhs) {
  auto lhs_type{lhs->GetQualType()};
  auto rhs_type{rhs->GetQualType()};

  if (lhs_type->PointerGetElementType()->IsVoidTy()) {
    lhs = Expr::MayCastTo(lhs, rhs_type);
  } else if (rhs_type->PointerGetElementType()->IsVoidTy()) {
    rhs = Expr::MayCastTo(rhs, lhs_type);
  }

  EnsureCompatible(lhs->GetType(), rhs->GetType());
}

Expr *Expr::MayCast(Expr *expr) {
  auto type{Type::MayCast(expr->GetQualType())};

  if (type != expr->GetQualType()) {
    return MakeAstNode<TypeCastExpr>(expr->GetLoc(), expr, type);
  } else {
    return expr;
  }
}

Expr *Expr::MayCastTo(Expr *expr, QualType to) {
  expr = Expr::MayCast(expr);

  if (!expr->GetType()->Equal(to.GetType())) {
    return MakeAstNode<TypeCastExpr>(expr->GetLoc(), expr, to);
  } else {
    return expr;
  }
}

Type *Expr::Convert(Expr *&lhs, Expr *&rhs) {
  auto type{QualType{ArithmeticType::MaxType(lhs->GetType(), rhs->GetType())}};

  lhs = Expr::MayCastTo(lhs, type);
  rhs = Expr::MayCastTo(rhs, type);

  return type.GetType();
}

bool Expr::IsZero(const Expr *expr) {
  if (auto constant{dynamic_cast<const ConstantExpr *>(expr)}) {
    if (constant->GetType()->IsIntegerTy() &&
        constant->GetIntegerVal().getSExtValue() == 0) {
      return true;
    }
  }

  return false;
}

Expr::Expr(QualType type) : type_{type} {}

/*
 * UnaryOpExpr
 */
UnaryOpExpr *UnaryOpExpr::Get(Tag tag, Expr *expr) {
  assert(expr != nullptr);
  return new (UnaryOpExprPool.malloc()) UnaryOpExpr{tag, expr};
}

AstNodeType UnaryOpExpr::Kind() const { return AstNodeType::kUnaryOpExpr; }

void UnaryOpExpr::Accept(Visitor &visitor) const { visitor.Visit(this); }

void UnaryOpExpr::Check() {
  switch (op_) {
  case Tag::kPlusPlus:
  case Tag::kMinusMinus:
  case Tag::kPostfixPlusPlus:
  case Tag::kPostfixMinusMinus:
    IncDecOpCheck();
    break;
  case Tag::kPlus:
  case Tag::kMinus:
    UnaryAddSubOpCheck();
    break;
  case Tag::kTilde:
    NotOpCheck();
    break;
  case Tag::kExclaim:
    LogicNotOpCheck();
    break;
  case Tag::kStar:
    DerefOpCheck();
    break;
  case Tag::kAmp:
    AddrOpCheck();
    break;
  default:
    assert(false);
  }
}

bool UnaryOpExpr::IsLValue() const { return op_ == Tag::kStar; }

Tag UnaryOpExpr::GetOp() const { return op_; }

const Expr *UnaryOpExpr::GetExpr() const { return expr_; }

UnaryOpExpr::UnaryOpExpr(Tag tag, Expr *expr) : op_(tag) {
  if (op_ == Tag::kAmp) {
    expr_ = expr;
  } else {
    expr_ = Expr::MayCast(expr);
  }
}

void UnaryOpExpr::IncDecOpCheck() {
  auto expr_type{expr_->GetQualType()};

  if (expr_->IsConst()) {
    Error(this, "cannot assign to something with const-qualified type");
  } else if (!expr_->IsLValue()) {
    Error(this, "lvalue expression expected");
  }

  if (expr_type->IsRealTy() || expr_type->IsPointerTy()) {
    type_ = expr_type;
  } else {
    Error(this, "expect operand of real or pointer type");
  }
}

void UnaryOpExpr::UnaryAddSubOpCheck() {
  if (!expr_->GetType()->IsArithmeticTy()) {
    Error(this, "expect operand of arithmetic type");
  }

  if (expr_->GetType()->IsIntegerOrBoolTy()) {
    expr_ = Expr::MayCastTo(expr_,
                            ArithmeticType::IntegerPromote(expr_->GetType()));
  }

  type_ = expr_->GetQualType();
}

void UnaryOpExpr::NotOpCheck() {
  if (!expr_->GetType()->IsIntegerTy()) {
    Error(this, "expect operand of arithmetic type");
  }

  expr_ =
      Expr::MayCastTo(expr_, ArithmeticType::IntegerPromote(expr_->GetType()));

  type_ = expr_->GetQualType();
}

void UnaryOpExpr::LogicNotOpCheck() {
  if (!expr_->GetType()->IsScalarTy()) {
    Error(this, "expect operand of scalar type");
  }

  type_ = ArithmeticType::Get(kInt);
}

void UnaryOpExpr::DerefOpCheck() {
  if (!expr_->GetType()->IsPointerTy()) {
    Error(this, "expect operand of pointer type");
  }

  type_ = expr_->GetType()->PointerGetElementType();
}

void UnaryOpExpr::AddrOpCheck() {
  if (!expr_->GetType()->IsFunctionTy() && !expr_->IsLValue()) {
    Error(this, "expression must be an lvalue or function");
  }

  type_ = PointerType::Get(expr_->GetQualType());
}

/*
 * TypeCastExpr
 */
TypeCastExpr *TypeCastExpr::Get(Expr *expr, QualType to) {
  assert(expr != nullptr);
  return new (TypeCastExprPool.malloc()) TypeCastExpr{expr, to};
}

AstNodeType TypeCastExpr::Kind() const { return AstNodeType::kTypeCastExpr; }

void TypeCastExpr::Accept(Visitor &visitor) const { visitor.Visit(this); }

void TypeCastExpr::Check() {
  if (type_->IsFloatPointTy() && expr_->GetType()->IsPointerTy()) {
    Error(loc_, "cannot cast a pointer to float point ('{}' to '{}')",
          expr_->GetQualType().ToString(), type_.ToString());
  } else if (type_->IsPointerTy() && expr_->GetType()->IsFloatPointTy()) {
    Error(loc_, "cannot cast a float point to pointer ('{}' to '{}')",
          expr_->GetQualType().ToString(), type_.ToString());
  }
}

bool TypeCastExpr::IsLValue() const { return false; }

const Expr *TypeCastExpr::GetExpr() const { return expr_; }

QualType TypeCastExpr::GetCastToType() const { return type_; }

TypeCastExpr::TypeCastExpr(Expr *expr, QualType to) : Expr(to), expr_{expr} {}

/*
 * BinaryOpExpr
 */
BinaryOpExpr *BinaryOpExpr::Get(Tag tag, Expr *lhs, Expr *rhs) {
  assert(lhs != nullptr && rhs != nullptr);
  return new (BinaryOpExprPool.malloc()) BinaryOpExpr{tag, lhs, rhs};
}

AstNodeType BinaryOpExpr::Kind() const { return AstNodeType::kBinaryOpExpr; }

void BinaryOpExpr::Accept(Visitor &visitor) const { visitor.Visit(this); }

void BinaryOpExpr::Check() {
  switch (op_) {
  case Tag::kEqual:
    AssignOpCheck();
    break;
  case Tag::kPlus:
    AddOpCheck();
    break;
  case Tag::kMinus:
    SubOpCheck();
    break;
  case Tag::kStar:
  case Tag::kPercent:
  case Tag::kSlash:
    MultiOpCheck();
    break;
  case Tag::kAmp:
  case Tag::kPipe:
  case Tag::kCaret:
    BitwiseOpCheck();
    break;
  case Tag::kLessLess:
  case Tag::kGreaterGreater:
    ShiftOpCheck();
    break;
  case Tag::kAmpAmp:
  case Tag::kPipePipe:
    LogicalOpCheck();
    break;
  case Tag::kExclaimEqual:
  case Tag::kEqualEqual:
    EqualityOpCheck();
    break;
  case Tag::kLess:
  case Tag::kLessEqual:
  case Tag::kGreater:
  case Tag::kGreaterEqual:
    RelationalOpCheck();
    break;
  case Tag::kPeriod:
    MemberRefOpCheck();
    break;
  case Tag::kComma:
    CommaOpCheck();
    break;
  default:
    assert(false);
  }
}

bool BinaryOpExpr::IsLValue() const {
  // 在 C++ 中赋值运算符表达式是左值表达式, 逗号运算符表达式可以是左值表达式
  // 而在 C 中两者都绝不是
  switch (op_) {
  case Tag::kPeriod:
    return lhs_->IsLValue();
  default:
    return false;
  }
}

Tag BinaryOpExpr::GetOp() const { return op_; }

const Expr *BinaryOpExpr::GetLHS() const { return lhs_; }

const Expr *BinaryOpExpr::GetRHS() const { return rhs_; }

BinaryOpExpr::BinaryOpExpr(Tag tag, Expr *lhs, Expr *rhs) : op_(tag) {
  if (tag != Tag::kPeriod) {
    lhs_ = MayCast(lhs);
    rhs_ = MayCast(rhs);
  } else {
    lhs_ = lhs;
    rhs_ = rhs;
  }
}

void BinaryOpExpr::AssignOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (lhs_->IsConst()) {
    Error(lhs_,
          "'{}': cannot assign to something with const-qualified type '{}'",
          magic_enum::enum_name(op_), lhs_type.ToString());
  } else if (!lhs_->IsLValue()) {
    Error(lhs_, "'{}': lvalue expression expected (got '{}')",
          magic_enum::enum_name(op_), lhs_type.ToString());
  }

  if (lhs_type->IsPointerTy() && rhs_type->IsIntegerTy()) {
    if (!Expr::IsZero(rhs_)) {
      Error(this, "must be pointer and zero");
    }
  }

  rhs_ = Expr::MayCastTo(rhs_, lhs_type);
  type_ = lhs_type;
}

void BinaryOpExpr::AddOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    type_ = Expr::Convert(lhs_, rhs_);
  } else if (lhs_type->IsIntegerTy() && rhs_type->IsPointerTy()) {
    lhs_ = Expr::MayCastTo(lhs_, ArithmeticType::Get(kLong | kUnsigned));
    type_ = rhs_type;
    // 保持 lhs 是指针
    std::swap(lhs_, rhs_);
  } else if (lhs_type->IsPointerTy() && rhs_type->IsIntegerTy()) {
    rhs_ = Expr::MayCastTo(rhs_, ArithmeticType::Get(kLong | kUnsigned));
    type_ = lhs_type;
  } else {
    Error(this, "the operand should be integer or pointer type");
  }
}

void BinaryOpExpr::SubOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    type_ = Expr::Convert(lhs_, rhs_);
  } else if (lhs_type->IsPointerTy() && rhs_type->IsIntegerTy()) {
    rhs_ = Expr::MayCastTo(rhs_, ArithmeticType::Get(kLong | kUnsigned));
    type_ = lhs_type;
  } else if (lhs_type->IsPointerTy() && rhs_type->IsPointerTy()) {
    // 如果这里有一个是 void* 指针也不能隐式转换
    EnsureCompatible(lhs_type, rhs_type);
    // ptrdiff_t
    type_ = QualType{ArithmeticType::Get(kLong)};
  } else {
    Error(this, "the operand should be integer or pointer type");
  }
}

void BinaryOpExpr::MultiOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (op_ == Tag::kPercent) {
    if (!lhs_type->IsIntegerTy() || !rhs_type->IsIntegerTy()) {
      Error(this, "the operand should be integer type");
    }
  } else {
    if (!lhs_type->IsArithmeticTy() || !rhs_type->IsArithmeticTy()) {
      Error(this, "the operand should be arithmetic type");
    }
  }

  type_ = Expr::Convert(lhs_, rhs_);
}

void BinaryOpExpr::BitwiseOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (!lhs_type->IsIntegerOrBoolTy() || !rhs_type->IsIntegerOrBoolTy()) {
    Error(this, "the operand should be Integer or bool type");
  }

  type_ = Expr::Convert(lhs_, rhs_);
}

void BinaryOpExpr::ShiftOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (!lhs_type->IsIntegerOrBoolTy() || !rhs_type->IsIntegerOrBoolTy()) {
    Error(this, "the operand should be Integer or bool type");
  }

  lhs_ =
      Expr::MayCastTo(lhs_, ArithmeticType::IntegerPromote(lhs_type.GetType()));
  // 本应该在每个运算数上独自进行整数提升, 但是 LLVM 指令要求类型一致
  // 通常右侧运算对象比较小, 这样应该并不会有什么问题
  rhs_ = Expr::MayCastTo(rhs_, lhs_->GetType());

  type_ = lhs_->GetQualType();
}

void BinaryOpExpr::LogicalOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (!lhs_type->IsScalarTy() || !rhs_type->IsScalarTy()) {
    Error(this, "the operand should be scalar type");
  }

  type_ = ArithmeticType::Get(kInt);
}

void BinaryOpExpr::EqualityOpCheck() {
  auto lhs_type{lhs_->GetType()};
  auto rhs_type{rhs_->GetType()};

  if (lhs_type->IsPointerTy() && rhs_type->IsPointerTy()) {
    EnsureCompatibleConvertVoidPtr(lhs_, rhs_);
  } else if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    Expr::Convert(lhs_, rhs_);
  } else if (lhs_type->IsPointerTy() && rhs_type->IsIntegerTy()) {
    if (!Expr::IsZero(rhs_)) {
      Error(this, "must be pointer and zero");
    } else {
      rhs_ = Expr::MayCastTo(rhs_, lhs_type);
    }
  } else if (lhs_type->IsIntegerTy() && rhs_type->IsPointerTy()) {
    std::swap(lhs_, rhs_);
    if (!Expr::IsZero(rhs_)) {
      Error(this, "must be pointer and zero");
    } else {
      rhs_ = Expr::MayCastTo(rhs_, rhs_type);
    }
  } else {
    Error(this, "the operand should be pointer or arithmetic type");
  }

  type_ = ArithmeticType::Get(kInt);
}

void BinaryOpExpr::RelationalOpCheck() {
  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (lhs_type->IsPointerTy() && rhs_type->IsPointerTy()) {
    EnsureCompatibleConvertVoidPtr(lhs_, rhs_);
  } else if (lhs_type->IsRealTy() && rhs_type->IsRealTy()) {
    Expr::Convert(lhs_, rhs_);
  } else {
    Error(this, "the operand should be pointer or real type");
  }

  type_ = ArithmeticType::Get(kInt);
}

void BinaryOpExpr::MemberRefOpCheck() { type_ = rhs_->GetQualType(); }

void BinaryOpExpr::CommaOpCheck() { type_ = rhs_->GetQualType(); }

/*
 * ConditionOpExpr
 */
ConditionOpExpr *ConditionOpExpr::Get(Expr *cond, Expr *lhs, Expr *rhs) {
  assert(cond != nullptr && lhs != nullptr && rhs != nullptr);
  return new (ConditionOpExprPool.malloc()) ConditionOpExpr{cond, lhs, rhs};
}

AstNodeType ConditionOpExpr::Kind() const {
  return AstNodeType::kConditionOpExpr;
}

void ConditionOpExpr::Accept(Visitor &visitor) const { visitor.Visit(this); }

void ConditionOpExpr::Check() {
  if (!cond_->GetType()->IsScalarTy()) {
    Error(loc_, "value of type '{}' is not contextually convertible to 'bool'",
          cond_->GetQualType().ToString());
  }

  auto lhs_type{lhs_->GetQualType()};
  auto rhs_type{rhs_->GetQualType()};

  if (lhs_type->IsArithmeticTy() && rhs_type->IsArithmeticTy()) {
    type_ = Expr::Convert(lhs_, rhs_);
  } else if (lhs_type->IsStructOrUnionTy() && rhs_type->IsStructOrUnionTy()) {
    if (!lhs_type->Equal(rhs_type.GetType())) {
      Error(loc_, "Must have the same struct or union type: '{}' vs '{}'",
            lhs_type.ToString(), rhs_type.ToString());
    }
    type_ = lhs_type;
  } else if (lhs_type->IsPointerTy() && rhs_type->IsPointerTy()) {
    // 这里放松了限制
    if (lhs_type->PointerGetElementType()->IsVoidTy()) {
      lhs_ = Expr::MayCastTo(lhs_, rhs_type);
    } else if (rhs_type->PointerGetElementType()->IsVoidTy()) {
      rhs_ = Expr::MayCastTo(rhs_, lhs_type);
    }
    type_ = lhs_->GetType();
  } else if (lhs_type->IsVoidTy() && rhs_type->IsVoidTy()) {
    type_ = VoidType::Get();
  } else if (lhs_type->IsPointerTy() && rhs_type->IsIntegerTy()) {
    if (!Expr::IsZero(rhs_)) {
      Error(this, "must be pointer and zero");
    } else {
      rhs_ = Expr::MayCastTo(rhs_, lhs_type);
    }
    type_ = lhs_type;
  } else if (lhs_type->IsIntegerTy() && rhs_type->IsPointerTy()) {
    if (!Expr::IsZero(lhs_)) {
      Error(this, "must be pointer and zero");
    } else {
      lhs_ = Expr::MayCastTo(lhs_, rhs_type);
    }
    type_ = rhs_type;
  } else {
    Error(loc_, "'?:': Type not allowed (got '{}' and '{}')",
          lhs_type.ToString(), rhs_type.ToString());
  }
}

bool ConditionOpExpr::IsLValue() const {
  // 和 C++ 不同，C 的条件运算符表达式绝不是左值
  return false;
}

const Expr *ConditionOpExpr::GetCond() const { return cond_; }

const Expr *ConditionOpExpr::GetLHS() const { return lhs_; }

const Expr *ConditionOpExpr::GetRHS() const { return rhs_; }

ConditionOpExpr::ConditionOpExpr(Expr *cond, Expr *lhs, Expr *rhs)
    : cond_(Expr::MayCast(cond)), lhs_(Expr::MayCast(lhs)),
      rhs_(Expr::MayCast(rhs)) {}

/*
 * FuncCallExpr
 */
FuncCallExpr *FuncCallExpr::Get(Expr *callee, std::vector<Expr *> args) {
  assert(callee != nullptr);
  return new (FuncCallExprPool.malloc()) FuncCallExpr{callee, std::move(args)};
}

AstNodeType FuncCallExpr::Kind() const { return AstNodeType::kFuncCallExpr; }

void FuncCallExpr::Accept(Visitor &visitor) const { visitor.Visit(this); }

void FuncCallExpr::Check() {
  if (callee_->GetType()->IsPointerTy()) {
    callee_ = MakeAstNode<UnaryOpExpr>(callee_->GetLoc(), Tag::kStar, callee_);
  }

  if (!callee_->GetType()->IsFunctionTy()) {
    Error(loc_, "called object is not a function or function pointer: {}",
          callee_->GetQualType().ToString());
  }

  auto func_type{callee_->GetType()};
  auto args_iter{std::begin(args_)};

  for (const auto &param : callee_->GetType()->FuncGetParams()) {
    assert(param != nullptr);

    if (args_iter == std::end(args_)) {
      Error(loc_, "too few arguments for function call");
    }

    *args_iter = Expr::MayCastTo(*args_iter, param->GetType());
    ++args_iter;
  }

  if (args_iter != std::end(args_) && !callee_->GetType()->FuncIsVarArgs()) {
    Error(loc_, "too many arguments for function call: (func type: '{}')",
          callee_->GetType()->ToString());
  }

  while (args_iter != std::end(args_)) {
    assert(*args_iter != nullptr);
    // 浮点提升
    if ((*args_iter)->GetType()->IsFloatTy()) {
      *args_iter = Expr::MayCastTo(
          *args_iter, QualType{ArithmeticType::Get(kDouble),
                               (*args_iter)->GetQualType().GetTypeQual()});
    } else if ((*args_iter)->GetQualType()->IsIntegerTy()) {
      *args_iter = Expr::MayCastTo(
          *args_iter, QualType{ArithmeticType::IntegerPromote(
                                   (*args_iter)->GetQualType().GetType()),
                               (*args_iter)->GetQualType().GetTypeQual()});
    }
    ++args_iter;
  }

  type_ = func_type->FuncGetReturnType();
}

bool FuncCallExpr::IsLValue() const { return false; }

Type *FuncCallExpr::GetFuncType() const { return callee_->GetType(); }

Expr *FuncCallExpr::GetCallee() const { return callee_; }

const std::vector<Expr *> &FuncCallExpr::GetArgs() const { return args_; }

void FuncCallExpr::SetVaArgType(Type *va_arg_type) {
  va_arg_type_ = va_arg_type;
}

Type *FuncCallExpr::GetVaArgType() const { return va_arg_type_; }

FuncCallExpr::FuncCallExpr(Expr *callee, std::vector<Expr *> args)
    : callee_{callee}, args_{std::move(args)} {}

/*
 * Constant
 */
ConstantExpr *ConstantExpr::Get(std::int32_t val) {
  return new (ConstantExprPool.malloc()) ConstantExpr{val};
}

ConstantExpr *ConstantExpr::Get(Type *type, std::uint64_t val) {
  assert(type != nullptr);
  return new (ConstantExprPool.malloc()) ConstantExpr{type, val};
}

ConstantExpr *ConstantExpr::Get(Type *type, const std::string &str) {
  assert(type != nullptr);
  return new (ConstantExprPool.malloc()) ConstantExpr{type, str};
}

AstNodeType ConstantExpr::Kind() const { return AstNodeType::kConstantExpr; }

void ConstantExpr::Accept(Visitor &visitor) const { visitor.Visit(this); }

void ConstantExpr::Check() {}

bool ConstantExpr::IsLValue() const { return false; }

const llvm::APInt &ConstantExpr::GetIntegerVal() const { return integer_val_; }

const llvm::APFloat &ConstantExpr::GetFloatPointVal() const {
  return float_point_val_;
}

ConstantExpr::ConstantExpr(std::int32_t val)
    : Expr(ArithmeticType::Get(kInt)), float_point_val_{llvm::APFloat{0.0}} {
  integer_val_ = llvm::APInt{type_->GetLLVMType()->getIntegerBitWidth(),
                             static_cast<std::uint64_t>(val), true};
}

ConstantExpr::ConstantExpr(Type *type, std::uint64_t val)
    : Expr(type), float_point_val_{llvm::APFloat{0.0}} {
  integer_val_ =
      llvm::APInt{type_->GetLLVMType()->getIntegerBitWidth(),
                  static_cast<std::uint64_t>(val), !type->IsUnsigned()};
}

ConstantExpr::ConstantExpr(Type *type, const std::string &str)
    : Expr(type), float_point_val_{llvm::APFloat{
                      GetFloatTypeSemantics(type->GetLLVMType()), str}} {}

/*
 * StringLiteral
 */
StringLiteralExpr *StringLiteralExpr::Get(const std::string &val) {
  return StringLiteralExpr::Get(ArithmeticType::Get(kChar), val);
}

StringLiteralExpr *StringLiteralExpr::Get(Type *type, const std::string &val) {
  assert(type != nullptr);
  return new (StringLiteralExprPool.malloc()) StringLiteralExpr{type, val};
}

AstNodeType StringLiteralExpr::Kind() const {
  return AstNodeType::kStringLiteralExpr;
}

void StringLiteralExpr::Accept(Visitor &visitor) const { visitor.Visit(this); }

void StringLiteralExpr::Check() {}

bool StringLiteralExpr::IsLValue() const { return false; }

const std::string &StringLiteralExpr::GetStr() const { return str_; }

llvm::Constant *StringLiteralExpr::GetArr() const { return Create().first; }

llvm::Constant *StringLiteralExpr::GetPtr() const { return Create().second; }

StringLiteralExpr::StringLiteralExpr(Type *type, const std::string &val)
    : Expr{ArrayType::Get(type, std::size(val) / type->GetWidth() + 1)},
      str_{val} {}

std::pair<llvm::Constant *, llvm::Constant *>
StringLiteralExpr::Create() const {
  auto iter{StringMap.find(str_)};
  if (iter != std::end(StringMap)) {
    return {iter->second.first, iter->second.second};
  }

  llvm::Constant *arr{}, *ptr{};
  auto width{type_->ArrayGetElementType()->GetWidth()};
  // 类型已经包含了空字符
  auto size{type_->ArrayGetNumElements() - 1};
  auto str{str_.c_str()};

  switch (width) {
  case 1: {
    std::vector<std::uint8_t> values;
    for (std::size_t i{}; i < size; ++i) {
      values.push_back(*reinterpret_cast<const std::uint8_t *>(str));
      str += 1;
    }
    // 空字符
    values.push_back(0);
    arr = llvm::ConstantDataArray::get(Context, values);
  } break;
  case 2: {
    std::vector<std::uint16_t> values;
    for (std::size_t i{}; i < size; ++i) {
      values.push_back(*reinterpret_cast<const std::uint16_t *>(str));
      str += 2;
    }
    values.push_back(0);
    arr = llvm::ConstantDataArray::get(Context, values);
  } break;
  case 4: {
    std::vector<std::uint32_t> values;
    for (std::size_t i{}; i < size; ++i) {
      values.push_back(*reinterpret_cast<const std::uint32_t *>(str));
      str += 4;
    }
    values.push_back(0);
    arr = llvm::ConstantDataArray::get(Context, values);
  } break;
  default:
    assert(false);
  }

  auto string{CreateGlobalString(arr, width)};

  auto zero{llvm::ConstantInt::get(Builder.getInt64Ty(), 0)};
  llvm::Constant *indices[]{zero, zero};
  ptr = llvm::ConstantExpr::getInBoundsGetElementPtr(nullptr, string, indices);

  StringMap[str_] = {arr, ptr};

  return {arr, ptr};
}

/*
 * Identifier
 */
IdentifierExpr *IdentifierExpr::Get(const std::string &name, QualType type,
                                    enum Linkage linkage, bool is_type_name) {
  return new (IdentifierExprPool.malloc())
      IdentifierExpr{name, type, linkage, is_type_name};
}

AstNodeType IdentifierExpr::Kind() const {
  return AstNodeType::kIdentifierExpr;
}

void IdentifierExpr::Accept(Visitor &visitor) const { visitor.Visit(this); }

void IdentifierExpr::Check() {}

bool IdentifierExpr::IsLValue() const { return false; }

enum Linkage IdentifierExpr::GetLinkage() const { return linkage_; }

const std::string &IdentifierExpr::GetName() const { return name_; }

bool IdentifierExpr::IsTypeName() const { return is_type_name_; }

bool IdentifierExpr::IsObject() const {
  return dynamic_cast<const ObjectExpr *>(this);
}

ObjectExpr *IdentifierExpr::ToObjectExpr() {
  return dynamic_cast<ObjectExpr *>(this);
}

const ObjectExpr *IdentifierExpr::ToObjectExpr() const {
  return dynamic_cast<const ObjectExpr *>(this);
}

IdentifierExpr::IdentifierExpr(const std::string &name, QualType type,
                               enum Linkage linkage, bool is_type_name)
    : Expr{type}, name_{name}, linkage_{linkage}, is_type_name_{is_type_name} {}

/*
 * Enumerator
 */
EnumeratorExpr *EnumeratorExpr::Get(const std::string &name, std::int32_t val) {
  return new (EnumeratorExprPool.malloc()) EnumeratorExpr{name, val};
}

AstNodeType EnumeratorExpr::Kind() const {
  return AstNodeType::kEnumeratorExpr;
}

void EnumeratorExpr::Accept(Visitor &visitor) const { visitor.Visit(this); }

void EnumeratorExpr::Check() {}

bool EnumeratorExpr::IsLValue() const { return false; }

std::int32_t EnumeratorExpr::GetVal() const { return val_; }

EnumeratorExpr::EnumeratorExpr(const std::string &name, std::int32_t val)
    : IdentifierExpr{name, ArithmeticType::Get(kInt), Linkage::kNone, false},
      val_{val} {}

/*
 * Object
 */
ObjectExpr *ObjectExpr::Get(const std::string &name, QualType type,
                            std::uint32_t storage_class_spec,
                            enum Linkage linkage, bool anonymous,
                            std::int32_t bit_field_width) {
  return new (ObjectExprPool.malloc()) ObjectExpr{
      name, type, storage_class_spec, linkage, anonymous, bit_field_width};
}

AstNodeType ObjectExpr::Kind() const { return AstNodeType::kObjectExpr; }

void ObjectExpr::Accept(Visitor &visitor) const { visitor.Visit(this); }

void ObjectExpr::Check() {}

bool ObjectExpr::IsLValue() const { return true; }

bool ObjectExpr::IsStatic() const { return storage_class_spec_ & kStatic; }

bool ObjectExpr::IsExtern() const { return storage_class_spec_ & kExtern; }

void ObjectExpr::SetStorageClassSpec(std::uint32_t storage_class_spec) {
  storage_class_spec_ = storage_class_spec;
}

std::uint32_t ObjectExpr::GetStorageClassSpec() { return storage_class_spec_; }

std::int32_t ObjectExpr::GetAlign() const { return align_; }

void ObjectExpr::SetAlign(std::int32_t align) { align_ = align; }

std::int32_t ObjectExpr::GetOffset() const { return offset_; }

void ObjectExpr::SetOffset(std::int32_t offset) { offset_ = offset; }

Declaration *ObjectExpr::GetDecl() const { return decl_; }

void ObjectExpr::SetDecl(Declaration *decl) { decl_ = decl; }

bool ObjectExpr::IsAnonymous() const { return anonymous_; }

bool ObjectExpr::IsGlobalVar() const { return linkage_ != Linkage::kNone; }

bool ObjectExpr::IsLocalStaticVar() const {
  return linkage_ == Linkage::kNone && IsStatic();
}

void ObjectExpr::SetLocalPtr(llvm::AllocaInst *local_ptr) {
  assert(local_ptr_ == nullptr);
  local_ptr_ = local_ptr;
}

llvm::AllocaInst *ObjectExpr::GetLocalPtr() const {
  assert(local_ptr_ != nullptr);
  return local_ptr_;
}

llvm::GlobalVariable *ObjectExpr::GetGlobalPtr() const {
  llvm::GlobalVariable *ptr{};

  if (IsAnonymous()) {
    ptr = new llvm::GlobalVariable(
        *Module, GetType()->GetLLVMType(), GetQualType().IsConst(),
        llvm::GlobalValue::InternalLinkage, GetDecl()->GetConstant(),
        ".compoundliteral");
  } else if (IsGlobalVar()) {
    ptr = CreateGlobalVar(this);
    return ptr;
  } else if (IsLocalStaticVar()) {
    assert(!std::empty(func_name_));
    auto name{func_name_ + "." + name_};

    if (auto iter{GlobalVarMap.find(name)}; iter != std::end(GlobalVarMap)) {
      ptr = iter->second;
    } else {
      ptr = new llvm::GlobalVariable(
          *Module, GetType()->GetLLVMType(), QualType().IsConst(),
          llvm::GlobalValue::InternalLinkage,
          GetConstantZero(GetType()->GetLLVMType()), name);
      GlobalVarMap[name] = ptr;
    }
  } else {
    assert(false);
  }

  // TODO align: int32 to uint64
  // Called C++ object pointer is null
  ptr->setAlignment(
      llvm::MaybeAlign{static_cast<std::uint64_t>(GetType()->GetAlign())});

  if (GetDecl()->HasConstantInit()) {
    ptr->setInitializer(GetDecl()->GetConstant());
  } else {
    if (!IsExtern()) {
      ptr->setInitializer(GetConstantZero(GetType()->GetLLVMType()));
    }
  }

  return ptr;
}

std::list<std::pair<Type *, std::int32_t>> &ObjectExpr::GetIndexs() {
  return indexs_;
}

const std::list<std::pair<Type *, std::int32_t>> &
ObjectExpr::GetIndexs() const {
  return indexs_;
}

std::int32_t ObjectExpr::GetBitFieldWidth() const { return bit_field_width_; }

std::int32_t ObjectExpr::GetBitFieldBegin() const { return bit_field_begin_; }

void ObjectExpr::SetBitFieldBegin(std::int32_t bit_field_begin) {
  bit_field_begin_ = bit_field_begin;
}

void ObjectExpr::SetType(Type *type) { type_ = type; }

void ObjectExpr::SetFuncName(const std::string &func_name) {
  func_name_ = func_name;
}

ObjectExpr::ObjectExpr(const std::string &name, QualType type,
                       std::uint32_t storage_class_spec, enum Linkage linkage,
                       bool anonymous, std::int32_t bit_field_width)
    : IdentifierExpr{name, type, linkage, false}, anonymous_{anonymous},
      storage_class_spec_{storage_class_spec}, align_{type->GetAlign()},
      bit_field_width_{bit_field_width} {}

/*
 * StmtExpr
 */
StmtExpr *StmtExpr::Get(CompoundStmt *block) {
  assert(block != nullptr);
  return new (StmtExprPool.malloc()) StmtExpr{block};
}

AstNodeType StmtExpr::Kind() const { return AstNodeType::kStmtExpr; }

void StmtExpr::Accept(Visitor &visitor) const { visitor.Visit(this); }

void StmtExpr::Check() {
  auto stmts{block_->GetStmts()};

  if (std::size(stmts) > 0 && stmts.back()->Kind() == AstNodeType::kExprStmt) {
    auto expr{dynamic_cast<ExprStmt *>(stmts.back())->GetExpr()};

    if (expr) {
      type_ = expr->GetQualType();
      return;
    }
  }

  type_ = VoidType::Get();
}

bool StmtExpr::IsLValue() const { return false; }

const CompoundStmt *StmtExpr::GetBlock() const { return block_; }

StmtExpr::StmtExpr(CompoundStmt *block) : block_{block} {}

/*
 * Stmt
 */
std::vector<Stmt *> Stmt::Children() const { return {}; }

/*
 * LabelStmt
 */
LabelStmt *LabelStmt::Get(const std::string &name, Stmt *stmt) {
  assert(stmt != nullptr);
  return new (LabelStmtPool.malloc()) LabelStmt{name, stmt};
}

AstNodeType LabelStmt::Kind() const { return AstNodeType::kLabelStmt; }

void LabelStmt::Accept(Visitor &visitor) const { visitor.Visit(this); }

void LabelStmt::Check() {}

std::vector<Stmt *> LabelStmt::Children() const { return {stmt_}; }

Stmt *LabelStmt::GetStmt() const { return stmt_; }

const std::string &LabelStmt::GetName() const { return name_; }

LabelStmt::LabelStmt(const std::string &name, Stmt *stmt)
    : name_{name}, stmt_{stmt} {}

/*
 * CaseStmt
 */
CaseStmt *CaseStmt::Get(std::int64_t lhs, Stmt *stmt) {
  assert(stmt != nullptr);
  return new (CaseStmtPool.malloc()) CaseStmt{lhs, stmt};
}

CaseStmt *CaseStmt::Get(std::int64_t lhs, std::int64_t rhs, Stmt *stmt) {
  assert(stmt != nullptr);
  return new (CaseStmtPool.malloc()) CaseStmt{lhs, rhs, stmt};
}

AstNodeType CaseStmt::Kind() const { return AstNodeType::kCaseStmt; }

void CaseStmt::Accept(Visitor &visitor) const { visitor.Visit(this); }

void CaseStmt::Check() {}

std::vector<Stmt *> CaseStmt::Children() const { return {stmt_}; }

std::int64_t CaseStmt::GetLHS() const { return lhs_; }

std::optional<std::int64_t> CaseStmt::GetRHS() const { return rhs_; }

const Stmt *CaseStmt::GetStmt() const { return stmt_; }

CaseStmt::CaseStmt(std::int64_t lhs, Stmt *stmt) : lhs_{lhs}, stmt_{stmt} {}

CaseStmt::CaseStmt(std::int64_t lhs, std::int64_t rhs, Stmt *stmt)
    : lhs_{lhs}, rhs_{rhs}, stmt_{stmt} {}

/*
 * DefaultStmt
 */
DefaultStmt *DefaultStmt::Get(Stmt *block) {
  assert(block != nullptr);
  return new (DefaultStmtPool.malloc()) DefaultStmt{block};
}

AstNodeType DefaultStmt::Kind() const { return AstNodeType::kDefaultStmt; }

void DefaultStmt::Accept(Visitor &visitor) const { visitor.Visit(this); }

void DefaultStmt::Check() {}

std::vector<Stmt *> DefaultStmt::Children() const { return {stmt_}; }

const Stmt *DefaultStmt::GetStmt() const { return stmt_; }

DefaultStmt::DefaultStmt(Stmt *block) : stmt_{block} {}

/*
 * CompoundStmt
 */
CompoundStmt *CompoundStmt::Get() {
  return new (CompoundStmtPool.malloc()) CompoundStmt{};
}

CompoundStmt *CompoundStmt::Get(std::vector<Stmt *> stmts) {
  return new (CompoundStmtPool.malloc()) CompoundStmt{std::move(stmts)};
}

AstNodeType CompoundStmt::Kind() const { return AstNodeType::kCompoundStmt; }

void CompoundStmt::Accept(Visitor &visitor) const { visitor.Visit(this); }

void CompoundStmt::Check() {}

std::vector<Stmt *> CompoundStmt::Children() const { return stmts_; }

const std::vector<Stmt *> &CompoundStmt::GetStmts() const { return stmts_; }

void CompoundStmt::AddStmt(Stmt *stmt) {
  // 非 typedef
  if (stmt) {
    stmts_.push_back(stmt);
  }
}

CompoundStmt::CompoundStmt(std::vector<Stmt *> stmts)
    : stmts_{std::move(stmts)} {}

/*
 * ExprStmt
 */
ExprStmt *ExprStmt::Get(Expr *expr) {
  return new (ExprStmtPool.malloc()) ExprStmt{expr};
}

AstNodeType ExprStmt::Kind() const { return AstNodeType::kExprStmt; }

void ExprStmt::Accept(Visitor &visitor) const { visitor.Visit(this); }

void ExprStmt::Check() {}

Expr *ExprStmt::GetExpr() const { return expr_; }

ExprStmt::ExprStmt(Expr *expr) : expr_{expr} {}

/*
 * IfStmt
 */
IfStmt *IfStmt::Get(Expr *cond, Stmt *then_block, Stmt *else_block) {
  assert(cond != nullptr && then_block != nullptr);
  return new (IfStmtPool.malloc()) IfStmt{cond, then_block, else_block};
}

AstNodeType IfStmt::Kind() const { return AstNodeType::kIfStmt; }

void IfStmt::Accept(Visitor &visitor) const { visitor.Visit(this); }

void IfStmt::Check() {
  if (!cond_->GetType()->IsScalarTy()) {
    Error(cond_->GetLoc(), "expect scalar type but got '{}'",
          cond_->GetQualType().ToString());
  }
}

std::vector<Stmt *> IfStmt::Children() const {
  return {then_block_, else_block_};
}

const Expr *IfStmt::GetCond() const { return cond_; }

const Stmt *IfStmt::GetThen() const { return then_block_; }

const Stmt *IfStmt::GetElse() const { return else_block_; }

IfStmt::IfStmt(Expr *cond, Stmt *then_block, Stmt *else_block)
    : cond_{Expr::MayCast(cond)}, then_block_{then_block}, else_block_{
                                                               else_block} {}

/*
 * SwitchStmt
 */
SwitchStmt *SwitchStmt::Get(Expr *cond, Stmt *block) {
  assert(cond != nullptr && block != nullptr);
  return new (SwitchStmtPool.malloc()) SwitchStmt{cond, block};
}

AstNodeType SwitchStmt::Kind() const { return AstNodeType::kSwitchStmt; }

void SwitchStmt::Accept(Visitor &visitor) const { visitor.Visit(this); }

void SwitchStmt::Check() {
  if (!cond_->GetType()->IsIntegerTy()) {
    Error(cond_->GetLoc(), "switch quantity not an integer (got '{}')",
          cond_->GetQualType().ToString());
  }

  cond_ = Expr::MayCastTo(cond_, ArithmeticType::Get(kLong));
}

std::vector<Stmt *> SwitchStmt::Children() const { return {stmt_}; }

const Expr *SwitchStmt::GetCond() const { return cond_; }

const Stmt *SwitchStmt::GetStmt() const { return stmt_; }

SwitchStmt::SwitchStmt(Expr *cond, Stmt *block) : cond_{cond}, stmt_{block} {}

/*
 * WhileStmt
 */
WhileStmt *WhileStmt::Get(Expr *cond, Stmt *block) {
  assert(cond != nullptr && block != nullptr);
  return new (WhileStmtPool.malloc()) WhileStmt{cond, block};
}

AstNodeType WhileStmt::Kind() const { return AstNodeType::kWhileStmt; }

void WhileStmt::Accept(Visitor &visitor) const { visitor.Visit(this); }

void WhileStmt::Check() {
  if (!cond_->GetType()->IsScalarTy()) {
    Error(cond_->GetLoc(), "expect scalar type but got '{}'",
          cond_->GetQualType().ToString());
  }
}

std::vector<Stmt *> WhileStmt::Children() const { return {block_}; }

const Expr *WhileStmt::GetCond() const { return cond_; }

const Stmt *WhileStmt::GetBlock() const { return block_; }

WhileStmt::WhileStmt(Expr *cond, Stmt *block)
    : cond_{Expr::MayCast(cond)}, block_{block} {}

/*
 * DoWhileStmt
 */
DoWhileStmt *DoWhileStmt::Get(Expr *cond, Stmt *block) {
  assert(cond != nullptr && block != nullptr);
  return new (DoWhileStmtPool.malloc()) DoWhileStmt{cond, block};
}

AstNodeType DoWhileStmt::Kind() const { return AstNodeType::kDoWhileStmt; }

void DoWhileStmt::Accept(Visitor &visitor) const { visitor.Visit(this); }

void DoWhileStmt::Check() {
  if (!cond_->GetType()->IsScalarTy()) {
    Error(cond_->GetLoc(), "expect scalar type but got '{}'",
          cond_->GetQualType().ToString());
  }
}

std::vector<Stmt *> DoWhileStmt::Children() const { return {block_}; }

const Expr *DoWhileStmt::GetCond() const { return cond_; }

const Stmt *DoWhileStmt::GetBlock() const { return block_; }

DoWhileStmt::DoWhileStmt(Expr *cond, Stmt *block)
    : cond_{Expr::MayCast(cond)}, block_{block} {}

/*
 * ForStmt
 */
ForStmt *ForStmt::Get(Expr *init, Expr *cond, Expr *inc, Stmt *block,
                      Stmt *decl) {
  return new (ForStmtPool.malloc()) ForStmt{init, cond, inc, block, decl};
}

AstNodeType ForStmt::Kind() const { return AstNodeType::kForStmt; }

void ForStmt::Accept(Visitor &visitor) const { visitor.Visit(this); }

void ForStmt::Check() {
  if (cond_ && !cond_->GetType()->IsScalarTy()) {
    Error(cond_->GetLoc(), "expect scalar but got '{}'",
          cond_->GetQualType().ToString());
  }
}

std::vector<Stmt *> ForStmt::Children() const { return {block_}; }

const Expr *ForStmt::GetInit() const { return init_; }

const Expr *ForStmt::GetCond() const { return cond_; }

const Expr *ForStmt::GetInc() const { return inc_; }

const Stmt *ForStmt::GetBlock() const { return block_; }

const Stmt *ForStmt::GetDecl() const { return decl_; }

ForStmt::ForStmt(Expr *init, Expr *cond, Expr *inc, Stmt *block, Stmt *decl)
    : init_{init}, cond_{cond}, inc_{inc}, block_{block}, decl_{decl} {}

/*
 * GotoStmt
 */
GotoStmt *GotoStmt::Get(const std::string &name) {
  return new (GotoStmtPool.malloc()) GotoStmt{name};
}

GotoStmt *GotoStmt::Get(LabelStmt *label) {
  assert(label != nullptr);
  return new (GotoStmtPool.malloc()) GotoStmt{label};
}

AstNodeType GotoStmt::Kind() const { return AstNodeType::kGotoStmt; }

void GotoStmt::Accept(Visitor &visitor) const { visitor.Visit(this); }

void GotoStmt::Check() {}

const LabelStmt *GotoStmt::GetLabel() const { return label_; }

void GotoStmt::SetLabel(LabelStmt *label) { label_ = label; }

const std::string &GotoStmt::GetName() const { return name_; }

GotoStmt::GotoStmt(const std::string &name) : name_{name} {}

GotoStmt::GotoStmt(LabelStmt *label) : label_{label} {}

/*
 * ContinueStmt
 */
ContinueStmt *ContinueStmt::Get() {
  return new (ContinueStmtPool.malloc()) ContinueStmt{};
}

AstNodeType ContinueStmt::Kind() const { return AstNodeType::kContinueStmt; }

void ContinueStmt::Accept(Visitor &visitor) const { visitor.Visit(this); }

void ContinueStmt::Check() {}

/*
 * BreakStmt
 */
BreakStmt *BreakStmt::Get() { return new (BreakStmtPool.malloc()) BreakStmt{}; }

AstNodeType BreakStmt::Kind() const { return AstNodeType::kBreakStmt; }

void BreakStmt::Accept(Visitor &visitor) const { visitor.Visit(this); }

void BreakStmt::Check() {}

/*
 * ReturnStmt
 */
ReturnStmt *ReturnStmt::Get(Expr *expr) {
  return new (ReturnStmtPool.malloc()) ReturnStmt{expr};
}

AstNodeType ReturnStmt::Kind() const { return AstNodeType::kReturnStmt; }

void ReturnStmt::Accept(Visitor &visitor) const { visitor.Visit(this); }

void ReturnStmt::Check() {}

const Expr *ReturnStmt::GetExpr() const { return expr_; }

ReturnStmt::ReturnStmt(Expr *expr) : expr_{expr} {}

/*
 * TranslationUnit
 */
TranslationUnit *TranslationUnit::Get() {
  return new (TranslationUnitPool.malloc()) TranslationUnit{};
}

AstNodeType TranslationUnit::Kind() const {
  return AstNodeType::kTranslationUnit;
}

void TranslationUnit::Accept(Visitor &visitor) const { visitor.Visit(this); }

void TranslationUnit::Check() {}

void TranslationUnit::AddExtDecl(ExtDecl *ext_decl) {
  // _Static_assert / e.g. int;
  if (ext_decl) {
    ext_decls_.push_back(ext_decl);
  }
}

const std::vector<ExtDecl *> &TranslationUnit::GetExtDecl() const {
  return ext_decls_;
}

/*
 * Initializer
 */
Initializer::Initializer(
    Type *type, Expr *expr,
    std::vector<std::tuple<Type *, std::int32_t, std::int32_t, std::int32_t>>
        indexs)
    : type_(type), expr_(expr), indexs_{std::move(indexs)} {
  assert(type != nullptr && expr != nullptr);
}

Type *Initializer::GetType() const {
  assert(type_ != nullptr);
  return type_;
}

Expr *&Initializer::GetExpr() {
  assert(expr_ != nullptr);
  return expr_;
}

const Expr *Initializer::GetExpr() const {
  assert(expr_ != nullptr);
  return expr_;
}

const std::vector<std::tuple<Type *, std::int32_t, std::int32_t, std::int32_t>>
    &Initializer::GetIndexs() const {
  return indexs_;
}

/*
 * Declaration
 */
Declaration *Declaration::Get(IdentifierExpr *ident) {
  assert(ident != nullptr);
  return new (DeclarationPool.malloc()) Declaration{ident};
}

AstNodeType Declaration::Kind() const { return AstNodeType::kDeclaration; }

void Declaration::Accept(Visitor &visitor) const { visitor.Visit(this); }

void Declaration::Check() {}

void Declaration::AddInits(std::vector<Initializer> inits) {
  if (std::size(inits) == 0) {
    // GNU 扩展
    value_init_ = true;
    return;
  }

  inits_ = std::move(inits);

  if (ident_->GetType()->IsScalarTy()) {
    assert(std::size(inits_) == 1);
    auto &init{inits_.front()};

    auto type{init.GetType()};
    auto &expr{init.GetExpr()};

    if (!type->Equal(expr->GetType())) {
      expr = Expr::MayCastTo(expr, type);
    }
  } else if (ident_->GetType()->IsAggregateTy()) {
    auto last{*(std::end(inits_) - 1)};

    for (auto &&init : inits_) {
      auto type{init.GetType()};
      auto &expr{init.GetExpr()};

      if (!type->Equal(expr->GetType())) {
        expr = Expr::MayCastTo(expr, type);
      }
    }
  }
}

const std::vector<Initializer> &Declaration::GetLocalInits() const {
  return inits_;
}

void Declaration::SetConstant(llvm::Constant *constant) {
  constant_ = constant;
}

llvm::Constant *Declaration::GetConstant() const { return constant_; }

bool Declaration::ValueInit() const { return value_init_; }

bool Declaration::HasLocalInit() const {
  return std::size(inits_) != 0 || value_init_;
}

bool Declaration::HasConstantInit() const {
  return constant_ != nullptr || value_init_;
}

IdentifierExpr *Declaration::GetIdent() const { return ident_; }

bool Declaration::IsObjDecl() const {
  return dynamic_cast<ObjectExpr *>(ident_);
}

ObjectExpr *Declaration::GetObject() const {
  assert(IsObjDecl());
  return dynamic_cast<ObjectExpr *>(ident_);
}

bool Declaration::IsObjDeclInGlobalOrLocalStatic() const {
  assert(IsObjDecl());
  auto obj{ident_->ToObjectExpr()};
  return obj->IsGlobalVar() || obj->IsLocalStaticVar();
}

Declaration::Declaration(IdentifierExpr *ident) : ident_{ident} {}

/*
 * FuncDef
 */
FuncDef *FuncDef::Get(IdentifierExpr *ident) {
  return new (FuncDefPool.malloc()) FuncDef{ident};
}

AstNodeType FuncDef::Kind() const { return AstNodeType::kFuncDef; }

void FuncDef::Accept(Visitor &visitor) const { visitor.Visit(this); }

void FuncDef::Check() {
  for (const auto &param : ident_->GetQualType()->FuncGetParams()) {
    assert(param != nullptr);
    if (param->IsAnonymous()) {
      Error(param, "param name omitted");
    }
  }
}

void FuncDef::SetBody(CompoundStmt *body) {
  if (ident_->GetType()->IsComplete()) {
    Error(ident_->GetLoc(), "redefinition of func {}", ident_->GetName());
  } else {
    body_ = body;
    ident_->GetType()->SetComplete(true);
  }
}

const std::string &FuncDef::GetName() const { return ident_->GetName(); }

enum Linkage FuncDef::GetLinkage() const { return ident_->GetLinkage(); }

Type *FuncDef::GetFuncType() const { return ident_->GetType(); }

IdentifierExpr *FuncDef::GetIdent() const { return ident_; }

const CompoundStmt *FuncDef::GetBody() const { return body_; }

FuncDef::FuncDef(IdentifierExpr *ident) : ident_{ident} {}

} // namespace kcc
