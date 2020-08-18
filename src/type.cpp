//
// Created by kaiser on 2019/10/31.
//

#include "type.h"

#include <algorithm>
#include <cassert>
#include <limits>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/Casting.h>

#include "ast.h"
#include "error.h"
#include "llvm_common.h"
#include "memory_pool.h"
#include "scope.h"

namespace kcc {

/*
 * QualType
 */
QualType::QualType(Type *type, std::uint32_t type_qual)
    : type_{type}, type_qual_{type_qual} {}

std::string QualType::ToString() const {
  std::string str;

  if (type_qual_ & kConst) {
    str += "const ";
  }
  if (type_qual_ & kRestrict) {
    str += "restrict ";
  }
  if (type_qual_ & kVolatile) {
    str += "volatile ";
  }
  if (type_qual_ & kAtomic) {
    assert(false);
  }

  return str + type_->ToString();
}

Type *QualType::operator->() {
  assert(type_ != nullptr);
  return type_;
}

const Type *QualType::operator->() const {
  assert(type_ != nullptr);
  return type_;
}

Type *QualType::GetType() {
  assert(type_ != nullptr);
  return type_;
}

const Type *QualType::GetType() const {
  assert(type_ != nullptr);
  return type_;
}

std::uint32_t QualType::GetTypeQual() const { return type_qual_; }

bool QualType::IsConst() const { return type_qual_ & kConst; }

bool QualType::IsVolatile() const { return type_qual_ & kVolatile; }

bool operator==(QualType lhs, QualType rhs) { return lhs.type_ == rhs.type_; }

bool operator!=(QualType lhs, QualType rhs) { return !(lhs == rhs); }

/*
 * Type
 */
QualType Type::MayCast(QualType type) {
  if (type->IsFunctionTy()) {
    return type->GetPointerTo();
  } else if (type->IsArrayTy()) {
    return type->ArrayGetElementType()->GetPointerTo();
  } else {
    return type;
  }
}

std::string Type::ToString() const {
  assert(llvm_type_ != nullptr);

  std::string prefix;

  if (IsIntegerTy()) {
    prefix = IsUnsigned() ? "u" : "";
  } else if (IsPointerTy()) {
    prefix = PointerGetElementType()->IsUnsigned() ? "u" : "";
  }

  return prefix + LLVMTypeToStr(llvm_type_);
}

llvm::Type *Type::GetLLVMType() const {
  assert(llvm_type_ != nullptr);
  return llvm_type_;
}

VoidType *Type::ToVoidType() { return dynamic_cast<VoidType *>(this); }

ArithmeticType *Type::ToArithmeticType() {
  return dynamic_cast<ArithmeticType *>(this);
}

PointerType *Type::ToPointerType() { return dynamic_cast<PointerType *>(this); }

ArrayType *Type::ToArrayType() { return dynamic_cast<ArrayType *>(this); }

StructType *Type::ToStructType() { return dynamic_cast<StructType *>(this); }

FunctionType *Type::ToFunctionType() {
  return dynamic_cast<FunctionType *>(this);
}

const VoidType *Type::ToVoidType() const {
  return dynamic_cast<const VoidType *>(this);
}

const ArithmeticType *Type::ToArithmeticType() const {
  return dynamic_cast<const ArithmeticType *>(this);
}

const PointerType *Type::ToPointerType() const {
  return dynamic_cast<const PointerType *>(this);
}

const ArrayType *Type::ToArrayType() const {
  return dynamic_cast<const ArrayType *>(this);
}

const StructType *Type::ToStructType() const {
  return dynamic_cast<const StructType *>(this);
}

const FunctionType *Type::ToFunctionType() const {
  return dynamic_cast<const FunctionType *>(this);
}

bool Type::IsComplete() const { return complete_; }

void Type::SetComplete(bool complete) {
  complete_ = complete;

  if (IsStructOrUnionTy()) {
    StructFinish();
  }
}

bool Type::IsUnsigned() const {
  if (IsBoolTy()) {
    return true;
  }

  if (!IsIntegerTy()) {
    return false;
  } else {
    return ToArithmeticType()->type_spec_ & kUnsigned;
  }
}

bool Type::IsVoidTy() const { return ToVoidType(); }

bool Type::IsBoolTy() const {
  auto type{ToArithmeticType()};
  return type && (type->type_spec_ == kBool);
}

bool Type::IsShortTy() const {
  auto type{ToArithmeticType()};
  return type && ((type->type_spec_ == kShort) ||
                  (type->type_spec_ == (kShort | kUnsigned)));
}

bool Type::IsIntTy() const {
  auto type{ToArithmeticType()};
  return type && ((type->type_spec_ == kInt) ||
                  (type->type_spec_ == (kInt | kUnsigned)));
}

bool Type::IsLongTy() const {
  auto type{ToArithmeticType()};
  return type && ((type->type_spec_ == kLong) ||
                  (type->type_spec_ == (kLong | kUnsigned)));
}

bool Type::IsLongLongTy() const {
  auto type{ToArithmeticType()};
  return type && ((type->type_spec_ == kLongLong) ||
                  (type->type_spec_ == (kLongLong | kUnsigned)));
}

bool Type::IsFloatTy() const {
  auto type{ToArithmeticType()};
  return type && (type->type_spec_ == kFloat);
}

bool Type::IsDoubleTy() const {
  auto type{ToArithmeticType()};
  return type && (type->type_spec_ == kDouble);
}

bool Type::IsLongDoubleTy() const {
  auto type{ToArithmeticType()};
  return type && (type->type_spec_ == (kLong | kDouble));
}

bool Type::IsPointerTy() const { return ToPointerType(); }

bool Type::IsArrayTy() const { return ToArrayType(); }

bool Type::IsStructTy() const {
  auto type{ToStructType()};
  return type && type->is_struct_;
}

bool Type::IsUnionTy() const {
  auto type{ToStructType()};
  return type && !type->is_struct_;
}

bool Type::IsStructOrUnionTy() const { return IsStructTy() || IsUnionTy(); }

bool Type::IsFunctionTy() const { return ToFunctionType(); }

bool Type::IsCharacterTy() const {
  auto type{ToArithmeticType()};
  return type && (type->type_spec_ & kChar);
}

bool Type::IsIntegerTy() const {
  return ToArithmeticType() && !IsFloatPointTy() && !IsBoolTy();
}

bool Type::IsRealTy() const { return IsIntegerTy() || IsRealFloatPointTy(); }

bool Type::IsArithmeticTy() const {
  return IsBoolTy() || IsIntegerTy() || IsFloatPointTy();
}

bool Type::IsScalarTy() const { return IsArithmeticTy() || IsPointerTy(); }

bool Type::IsAggregateTy() const { return IsArrayTy() || IsStructOrUnionTy(); }

bool Type::IsRealFloatPointTy() const {
  return IsFloatTy() || IsDoubleTy() || IsLongDoubleTy();
}

bool Type::IsFloatPointTy() const {
  // 不支持虚浮点数
  return IsRealFloatPointTy();
}

bool Type::IsIntegerOrBoolTy() const { return IsIntegerTy() || IsBoolTy(); }

PointerType *Type::GetPointerTo() { return PointerType::Get(this); }

std::int32_t Type::ArithmeticRank() const {
  assert(IsArithmeticTy());
  return ToArithmeticType()->Rank();
}

std::uint64_t Type::ArithmeticMaxIntegerValue() const {
  assert(IsArithmeticTy());
  return ToArithmeticType()->MaxIntegerValue();
}

QualType Type::PointerGetElementType() const {
  assert(IsPointerTy());
  return ToPointerType()->GetElementType();
}

void Type::ArraySetNumElements(std::size_t num_elements) {
  assert(IsArrayTy());
  ToArrayType()->SetNumElements(num_elements);
}

std::size_t Type::ArrayGetNumElements() const {
  assert(IsArrayTy());
  return ToArrayType()->GetNumElements();
}

QualType Type::ArrayGetElementType() const {
  assert(IsArrayTy());
  return ToArrayType()->GetElementType();
}

bool Type::StructHasName() const {
  assert(IsStructOrUnionTy());
  return ToStructType()->HasName();
}

void Type::StructSetName(const std::string &name) {
  assert(IsStructOrUnionTy());
  ToStructType()->SetName(name);
}

const std::string &Type::StructGetName() const {
  assert(IsStructOrUnionTy());
  return ToStructType()->GetName();
}

std::vector<ObjectExpr *> &Type::StructGetMembers() {
  assert(IsStructOrUnionTy());
  return ToStructType()->GetMembers();
}

const std::vector<ObjectExpr *> &Type::FuncGetParams() const {
  assert(IsStructOrUnionTy());
  return ToStructType()->GetMembers();
}

void Type::StructSetMembers(std::vector<ObjectExpr *> &members) {
  assert(IsStructOrUnionTy());
  ToStructType()->SetMembers(members);
}

ObjectExpr *Type::StructGetMember(const std::string &name) const {
  assert(IsStructOrUnionTy());
  return ToStructType()->GetMember(name);
}

QualType Type::StructGetMemberType(std::int32_t i) const {
  assert(IsStructOrUnionTy());
  return ToStructType()->GetMemberType(i);
}

Scope *Type::StructGetScope() {
  assert(IsStructOrUnionTy());
  return ToStructType()->GetScope();
}

void Type::StructAddMember(ObjectExpr *member) {
  assert(IsStructOrUnionTy());
  ToStructType()->AddMember(member);
}

void Type::StructMergeAnonymous(ObjectExpr *anonymous) {
  assert(IsStructOrUnionTy());
  ToStructType()->MergeAnonymous(anonymous);
}

std::int32_t Type::StructGetOffset() const {
  assert(IsStructOrUnionTy());
  return ToStructType()->GetOffset();
}

void Type::StructFinish() {
  assert(IsStructOrUnionTy());
  ToStructType()->Finish();
}

bool Type::FuncIsVarArgs() const {
  assert(IsFunctionTy());
  return ToFunctionType()->IsVarArgs();
}

QualType Type::FuncGetReturnType() const {
  assert(IsFunctionTy());
  return ToFunctionType()->GetReturnType();
}

std::vector<ObjectExpr *> &Type::FuncGetParams() {
  assert(IsFunctionTy());
  return ToFunctionType()->GetParams();
}

const std::vector<ObjectExpr *> &Type::StructGetMembers() const {
  assert(IsFunctionTy());
  return ToFunctionType()->GetParams();
}

void Type::FuncSetFuncSpec(std::uint32_t func_spec) {
  assert(IsFunctionTy());
  return ToFunctionType()->SetFuncSpec(func_spec);
}

bool Type::FuncIsInline() const {
  assert(IsFunctionTy());
  return ToFunctionType()->IsInline();
}

void Type::FuncSetName(const std::string &name) {
  assert(IsFunctionTy());
  ToFunctionType()->SetName(name);
}

const std::string &Type::FuncGetName() const {
  assert(IsFunctionTy());
  return ToFunctionType()->GetName();
}

Type::Type(bool complete) : complete_{complete} {}

/*
 * VoidType
 */
VoidType *VoidType::Get() {
  static auto type{new (VoidTypePool.malloc()) VoidType{}};
  return type;
}

std::int32_t VoidType::GetWidth() const {
  // GNU 扩展
  return 1;
}

std::int32_t VoidType::GetAlign() const { return GetWidth(); }

bool VoidType::Compatible(const Type *other) const { return Equal(other); }

bool VoidType::Equal(const Type *other) const { return other->IsVoidTy(); }

VoidType::VoidType() : Type{false} { llvm_type_ = Builder.getVoidTy(); }

/*
 * ArithmeticType
 */
ArithmeticType *ArithmeticType::Get(std::uint32_t type_spec) {
  static auto bool_type{new (ArithmeticTypePool.malloc())
                            ArithmeticType{kBool}};
  static auto char_type{new (ArithmeticTypePool.malloc())
                            ArithmeticType{kChar}};
  static auto uchar_type{new (ArithmeticTypePool.malloc())
                             ArithmeticType{kChar | kUnsigned}};
  static auto short_type{new (ArithmeticTypePool.malloc())
                             ArithmeticType{kShort}};
  static auto ushort_type{new (ArithmeticTypePool.malloc())
                              ArithmeticType{kShort | kUnsigned}};
  static auto int_type{new (ArithmeticTypePool.malloc()) ArithmeticType{kInt}};
  static auto uint_type{new (ArithmeticTypePool.malloc())
                            ArithmeticType{kInt | kUnsigned}};
  static auto long_type{new (ArithmeticTypePool.malloc())
                            ArithmeticType{kLong}};
  static auto ulong_type{new (ArithmeticTypePool.malloc())
                             ArithmeticType{kLong | kUnsigned}};
  static auto long_long_type{new (ArithmeticTypePool.malloc())
                                 ArithmeticType{kLongLong}};
  static auto ulong_long_type{new (ArithmeticTypePool.malloc())
                                  ArithmeticType{kLongLong | kUnsigned}};
  static auto float_type{new (ArithmeticTypePool.malloc())
                             ArithmeticType{kFloat}};
  static auto double_type{new (ArithmeticTypePool.malloc())
                              ArithmeticType{kDouble}};
  static auto long_double_type{new (ArithmeticTypePool.malloc())
                                   ArithmeticType{kDouble | kLong}};

  type_spec = ArithmeticType::DealWithTypeSpec(type_spec);

  switch (type_spec) {
  case kBool:
    return bool_type;
  case kChar:
    return char_type;
  case kChar | kUnsigned:
    return uchar_type;
  case kShort:
    return short_type;
  case kShort | kUnsigned:
    return ushort_type;
  case kInt:
    return int_type;
  case kInt | kUnsigned:
    return uint_type;
  case kLong:
    return long_type;
  case kLong | kUnsigned:
    return ulong_type;
  case kLongLong:
    return long_long_type;
  case kLongLong | kUnsigned:
    return ulong_long_type;
  case kFloat:
    return float_type;
  case kDouble:
    return double_type;
  case kDouble | kLong:
    return long_double_type;
  default:
    assert(false);
    return nullptr;
  }
}

Type *ArithmeticType::IntegerPromote(Type *type) {
  assert(type != nullptr);
  assert(type->IsIntegerTy() || type->IsBoolTy());

  static auto int_type{ArithmeticType::Get(kInt)};

  if (type->ArithmeticRank() < int_type->Rank()) {
    return int_type;
  } else {
    return type;
  }
}

Type *ArithmeticType::MaxType(Type *lhs, Type *rhs) {
  assert(lhs != nullptr && rhs != nullptr);
  assert(lhs->IsArithmeticTy() && rhs->IsArithmeticTy());

  if ((!lhs->IsIntegerTy() || !rhs->IsIntegerTy()) &&
      (!lhs->IsBoolTy() && !rhs->IsBoolTy())) {
    return lhs->ArithmeticRank() >= rhs->ArithmeticRank() ? lhs : rhs;
  } else {
    lhs = ArithmeticType::IntegerPromote(lhs);
    rhs = ArithmeticType::IntegerPromote(rhs);

    if (lhs->Equal(rhs)) {
      return lhs;
    } else if (lhs->IsUnsigned() == rhs->IsUnsigned()) {
      return lhs->ArithmeticRank() >= rhs->ArithmeticRank() ? lhs : rhs;
    } else {
      auto unsigned_type{lhs->IsUnsigned() ? lhs : rhs};
      auto signed_type{lhs->IsUnsigned() ? rhs : lhs};

      if (unsigned_type->ArithmeticRank() >= signed_type->ArithmeticRank()) {
        return unsigned_type;
      } else {
        if (signed_type->ArithmeticMaxIntegerValue() >=
            unsigned_type->ArithmeticMaxIntegerValue()) {
          return signed_type;
        } else {
          return ArithmeticType::Get(
              signed_type->ToArithmeticType()->type_spec_ | kUnsigned);
        }
      }
    }
  }
}

std::int32_t ArithmeticType::GetWidth() const {
  switch (type_spec_) {
  case kBool:
    return 1;
  case kChar:
  case kChar | kUnsigned:
    return 1;
  case kShort:
  case kShort | kUnsigned:
    return 2;
  case kInt:
  case kInt | kUnsigned:
    return 4;
  case kLong:
  case kLong | kUnsigned:
  case kLongLong:
  case kLongLong | kUnsigned:
    return 8;
  case kFloat:
    return 4;
  case kDouble:
    return 8;
  case kDouble | kLong:
    return 16;
  default:
    assert(false);
    return 0;
  }
}

std::int32_t ArithmeticType::GetAlign() const { return GetWidth(); }

// 它们是同一类型(同名或由 typedef 引入的别名)
// 类型 char 不与 signed char 兼容, 且不与 unsigned char 兼容(未实现)
bool ArithmeticType::Compatible(const Type *other) const {
  assert(other != nullptr);
  return Equal(other);
}

bool ArithmeticType::Equal(const Type *other) const {
  assert(other != nullptr);

  if (other->IsArithmeticTy()) {
    return type_spec_ == other->ToArithmeticType()->type_spec_;
  } else {
    return false;
  }
}

std::uint64_t ArithmeticType::MaxIntegerValue() const {
  switch (type_spec_) {
  case kChar:
    return std::numeric_limits<std::int8_t>::max();
  case kChar | kUnsigned:
    return std::numeric_limits<std::uint8_t>::max();
  case kShort:
    return std::numeric_limits<std::int16_t>::max();
  case kShort | kUnsigned:
    return std::numeric_limits<std::uint16_t>::max();
  case kInt:
    return std::numeric_limits<std::int32_t>::max();
  case kInt | kUnsigned:
    return std::numeric_limits<std::uint32_t>::max();
  case kLong:
  case kLongLong:
    return std::numeric_limits<std::int64_t>::max();
  case kLong | kUnsigned:
  case kLongLong | kUnsigned:
    return std::numeric_limits<std::uint64_t>::max();
  default:
    assert(false);
    return 0;
  }
}

ArithmeticType::ArithmeticType(std::uint32_t type_spec) : Type{true} {
  type_spec_ = ArithmeticType::DealWithTypeSpec(type_spec);

  if (IsBoolTy()) {
    llvm_type_ = Builder.getInt1Ty();
  } else if (IsCharacterTy()) {
    llvm_type_ = Builder.getInt8Ty();
  } else if (IsShortTy()) {
    llvm_type_ = Builder.getInt16Ty();
  } else if (IsIntTy()) {
    llvm_type_ = Builder.getInt32Ty();
  } else if (IsLongTy() || IsLongLongTy()) {
    llvm_type_ = Builder.getInt64Ty();
  } else if (IsFloatTy()) {
    llvm_type_ = Builder.getFloatTy();
  } else if (IsDoubleTy()) {
    llvm_type_ = Builder.getDoubleTy();
  } else if (IsLongDoubleTy()) {
    llvm_type_ = llvm::Type::getX86_FP80Ty(Context);
  } else {
    assert(false);
  }
}

std::uint32_t ArithmeticType::DealWithTypeSpec(std::uint32_t type_spec) {
  if (type_spec == kSigned) {
    type_spec = kInt;
  } else if (type_spec == kUnsigned) {
    type_spec |= kInt;
  }

  type_spec &= ~kSigned;

  if ((type_spec & kShort) || (type_spec & kLong) || (type_spec & kLongLong)) {
    type_spec &= ~kInt;
  }

  return type_spec;
}

std::int32_t ArithmeticType::Rank() const {
  switch (type_spec_) {
  case kBool:
    return 0;
  case kChar:
  case kChar | kUnsigned:
    return 1;
  case kShort:
  case kShort | kUnsigned:
    return 2;
  case kInt:
  case kInt | kUnsigned:
    return 3;
  case kLong:
  case kLong | kUnsigned:
    return 4;
  case kLongLong:
  case kLongLong | kUnsigned:
    return 5;
  case kFloat:
    return 6;
  case kDouble:
    return 7;
  case kDouble | kLong:
    return 8;
  default:
    assert(false);
    return 0;
  }
}

/*
 * PointerType
 */
PointerType *PointerType::Get(QualType element_type) {
  return new (PointerTypePool.malloc()) PointerType{element_type};
}

std::int32_t PointerType::GetWidth() const { return 8; }

std::int32_t PointerType::GetAlign() const { return GetWidth(); }

// 指向兼容类型
bool PointerType::Compatible(const Type *other) const {
  assert(other != nullptr);

  if (other->IsPointerTy()) {
    return element_type_->Compatible(
        other->ToPointerType()->element_type_.GetType());
  } else {
    return false;
  }
}

bool PointerType::Equal(const Type *other) const {
  assert(other != nullptr);

  if (other->IsPointerTy()) {
    return element_type_->Equal(
        other->ToPointerType()->element_type_.GetType());
  } else {
    return false;
  }
}

QualType PointerType::GetElementType() const { return element_type_; }

PointerType::PointerType(QualType element_type)
    : Type{true}, element_type_{element_type} {
  if (element_type_->IsVoidTy()) {
    llvm_type_ = Builder.getInt8PtrTy();
  } else {
    llvm_type_ = element_type_->GetLLVMType()->getPointerTo();
  }
}

/*
 * ArrayType
 */
ArrayType *ArrayType::Get(QualType contained_type,
                          std::optional<std::size_t> num_elements) {
  return new (ArrayTypePool.malloc()) ArrayType{contained_type, num_elements};
}

std::int32_t ArrayType::GetWidth() const {
  assert(num_elements_);
  return contained_type_->GetWidth() * *num_elements_;
}

std::int32_t ArrayType::GetAlign() const { return contained_type_->GetAlign(); }

// 其元素类型兼容, 且
// 若都拥有常量大小, 则大小相同
// 注意: 未知边界数组与任何兼容元素类型的数组兼容
bool ArrayType::Compatible(const Type *other) const {
  assert(other != nullptr);

  if (other->IsArrayTy()) {
    auto other_arr{other->ToArrayType()};
    if (!contained_type_->Compatible(other_arr->contained_type_.GetType())) {
      return false;
    }
    if (IsComplete() && other_arr->IsComplete()) {
      return GetNumElements() == other_arr->GetNumElements();
    }

    return true;
  } else {
    return false;
  }
}

bool ArrayType::Equal(const Type *other) const {
  assert(other != nullptr);

  if (other->IsArrayTy()) {
    auto other_arr{other->ToArrayType()};
    if (!contained_type_->Equal(other_arr->contained_type_.GetType())) {
      return false;
    }
    if (IsComplete() && other_arr->IsComplete()) {
      return GetNumElements() == other_arr->GetNumElements();
    }

    return true;
  } else {
    return false;
  }
}

void ArrayType::SetNumElements(std::size_t num_elements) {
  assert(num_elements > 0);
  num_elements_ = num_elements;
  // 在对元素数量未知的全局或静态数组初始化时, 如果不及时更新 LLVM 类型,
  // 将会导致用于初始化的 llvm::Constant 类型不正确
  llvm_type_ =
      llvm::ArrayType::get(contained_type_->GetLLVMType(), num_elements);
}

std::size_t ArrayType::GetNumElements() const {
  if (num_elements_) {
    return *num_elements_;
  } else {
    return 0;
  }
}

QualType ArrayType::GetElementType() const { return contained_type_; }

ArrayType::ArrayType(QualType contained_type,
                     std::optional<std::size_t> num_elements)
    : Type{num_elements.has_value()}, contained_type_{contained_type},
      num_elements_{num_elements} {
  if (num_elements.has_value()) {
    llvm_type_ =
        llvm::ArrayType::get(contained_type_->GetLLVMType(), *num_elements_);
  } else {
    llvm_type_ = llvm::ArrayType::get(contained_type_->GetLLVMType(), 0);
  }
}

/*
 * StructType
 */
StructType *StructType::Get(bool is_struct, const std::string &name,
                            Scope *parent) {
  return new (StructTypePool.malloc()) StructType{is_struct, name, parent};
}

std::int32_t StructType::GetWidth() const {
  assert(IsComplete());

  auto struct_type{llvm::cast<llvm::StructType>(llvm_type_)};
  return Module->getDataLayout().getStructLayout(struct_type)->getSizeInBytes();
}

std::int32_t StructType::GetAlign() const {
  if (!IsComplete()) {
    return 1;
  }

  auto struct_type{llvm::cast<llvm::StructType>(llvm_type_)};
  return Module->getDataLayout()
      .getStructLayout(struct_type)
      ->getAlignment()
      .value();
}

// 若一者以标签声明, 则另一者必须以同一标签声明。
// 若它们都是完整类型, 则其成员必须在数量上准确对应, 以兼容类型声明,
// 并拥有匹配的名称 另外, 若它们都是枚举, 则对应成员亦必须拥有相同值。 另外,
// 若它们是结构体或联合体, 则 对应的元素必须以同一顺序声明(仅结构体)
bool StructType::Compatible(const Type *other) const {
  assert(other != nullptr);

  if (other->IsStructOrUnionTy() && IsStruct() == other->IsStructTy()) {
    auto other_struct{other->ToStructType()};
    if (HasName() && other_struct->HasName()) {
      if (name_ != other_struct->name_) {
        return false;
      }
    }

    if (IsComplete() && other_struct->IsComplete()) {
      if (GetNumMembers() != other_struct->GetNumMembers()) {
        return false;
      }

      return GetLLVMType() == other_struct->GetLLVMType();
    }

    return true;
  } else {
    return false;
  }
}

bool StructType::Equal(const Type *other) const {
  assert(other != nullptr);

  if (other->IsStructOrUnionTy() && IsStruct() == other->IsStructTy()) {
    auto other_struct{other->ToStructType()};
    if (HasName() && other_struct->HasName()) {
      if (name_ != other_struct->name_) {
        return false;
      }
    }

    if (IsComplete() && other_struct->IsComplete()) {
      if (GetNumMembers() != other_struct->GetNumMembers()) {
        return false;
      }

      return GetLLVMType() == other_struct->GetLLVMType();
    }

    return true;
  } else {
    return false;
  }
}

bool StructType::IsStruct() const { return is_struct_; }

bool StructType::HasName() const { return !std::empty(name_); }

void StructType::SetName(const std::string &name) {
  name_ = name;
  std::string prefix{is_struct_ ? "struct." : "union."};
  llvm::cast<llvm::StructType>(llvm_type_)->setName(prefix + name);
}

const std::string &StructType::GetName() const { return name_; }

std::int32_t StructType::GetNumMembers() const { return std::size(members_); }

std::vector<ObjectExpr *> &StructType::GetMembers() { return members_; }

const std::vector<ObjectExpr *> &StructType::GetMembers() const {
  return members_;
}

void StructType::SetMembers(std::vector<ObjectExpr *> &members) {
  members_ = members;
}

ObjectExpr *StructType::GetMember(const std::string &name) const {
  auto ident{scope_->FindUsualInCurrScope(name)};
  if (ident == nullptr) {
    return nullptr;
  } else {
    assert(ident->IsObject());
    return ident->ToObjectExpr();
  }
}

QualType StructType::GetMemberType(std::int32_t i) const {
  return members_[i]->GetQualType();
}

Scope *StructType::GetScope() { return scope_; }

std::int32_t StructType::GetOffset() const { return offset_; }

void StructType::AddMember(ObjectExpr *member) {
  align_ = std::max(align_, member->GetAlign());
  // 上一个字段是位域并且尚未将类型添加
  AddBitFieldBeforeMember();

  auto type{member->GetType()};
  // 柔性数组
  if (type->IsArrayTy() && !type->IsComplete()) {
    type = ArrayType::Get(type->ArrayGetElementType(), 0);
    member->SetType(type);
  }

  auto offset{MakeAlign(offset_, member->GetAlign())};
  // bit field 前后的对齐空间不包括在 offset 中
  member->SetOffset(offset - bit_field_space_count_);

  member->GetIndexs().push_front({this, index_++});

  members_.push_back(member);
  scope_->InsertUsual(member);

  AddLLVMType(type);
  if (is_struct_) {
    offset_ = offset + type->GetWidth();
    width_ = MakeAlign(offset_, align_);
  } else {
    assert(offset_ == 0);
    width_ = std::max(width_, type->GetWidth());
    width_ = MakeAlign(width_, align_);
  }
}

// 匿名 struct / union
void StructType::MergeAnonymous(ObjectExpr *anonymous) {
  align_ = std::max(align_, anonymous->GetAlign());
  AddBitFieldBeforeMember();

  assert(anonymous->GetType()->IsStructOrUnionTy());
  auto anonymous_type{anonymous->GetType()->ToStructType()};

  auto offset{MakeAlign(offset_, anonymous->GetAlign())};
  anonymous->SetOffset(offset);

  anonymous->GetIndexs().push_front({this, index_});

  members_.push_back(anonymous);

  for (auto &&[name, obj] : *anonymous_type->scope_) {
    if (auto member{obj->ToObjectExpr()}; !member) {
      continue;
    } else {
      if (GetMember(name)) {
        Error(member->GetLoc(), "duplicated member: '{}'", name);
      }

      member->SetOffset(offset + member->GetOffset());

      scope_->InsertUsual(member);
      member->GetIndexs().push_front({this, index_});
    }
  }

  ++index_;

  AddLLVMType(anonymous_type);
  if (is_struct_) {
    offset_ = offset + anonymous->GetType()->GetWidth();
    width_ = MakeAlign(offset_, align_);
  } else {
    assert(offset_ == 0);
    width_ = std::max(width_, anonymous->GetType()->GetWidth());
    width_ = MakeAlign(width_, align_);
  }
}

// 1) 如果存在相邻位域, 且位宽之和小于类型的 sizeof 大小
// 则后一个字段将紧邻前一个字段存储, 直到不能容纳为止
// 2) 如果存在相邻位域, 但位宽之和大于类型的sizeof大小,
// 则后一个字段将从新的存储单元开始, 其偏移量为其类型大小的整数倍
// 3) 如果位域字段之间穿插着非位域字段, 则不进行打包
// 4) 拥有零 width 的特殊无名位域打破填充:
// 它指定下个位域在始于下个分配单元的起点
void StructType::AddBitField(ObjectExpr *member) {
  if (member->GetBitFieldWidth() == 0 || member->GetBitFieldWidth() > 8) {
    align_ = std::max(member->GetAlign(), align_);
  }

  auto bit_width{member->GetBitFieldWidth()};

  // 该成员是第一个字段或上一个字段不是位域或已经没有位置可以打包了
  if (bit_field_used_width_ == 0) {
    // char a[3];
    // int : 9;
    // int c;
    // [3 x i8], i8, [2 x i8], [2 x i8], i32

    // char a[3];
    // int : 5;
    // int c;
    // [3 x i8], i8, i32
    if (is_struct_ && !std::empty(members_)) {
      auto last{members_.back()};
      if (!last->GetBitFieldWidth()) {
        width_ = MakeAlign(offset_, align_);
        AddSpace(width_ - offset_);
      }
    }

    assert(bit_field_base_type_width_ == 0);
    bit_field_base_type_width_ = member->GetType()->GetWidth() * 8;

    // 这时使用 :0 没有效果
    if (bit_width == 0) {
      return;
    }

    member->SetOffset(offset_);
    member->SetBitFieldBegin(0);

    // 这里不递增 index_
    member->GetIndexs().push_front({this, index_});

    members_.push_back(member);
    scope_->InsertUsual(member);

    // 如果是 struct , 当打包满了或者下一个不是位域字段时再改变 offset_
    if (!is_struct_) {
      UnionAddBitField(member->GetType());
    }

    bit_field_used_width_ += bit_width;

    assert(bit_field_used_width_ <= bit_field_base_type_width_);
    // 满了
    if (bit_field_used_width_ == bit_field_base_type_width_) {
      BitFieldPacked();
    }
  } else {
    if (bit_width == 0) {
      if (IsUnionTy() || member->GetType()->IsCharacterTy()) {
        return;
      }

      AddSpaceBetweenBitField();
    } else {
      auto new_bit_field_base_type_width{member->GetType()->GetWidth() * 8};
      if (new_bit_field_base_type_width > bit_field_base_type_width_) {
        bit_field_base_type_width_ = new_bit_field_base_type_width;
      }

      if (bit_field_used_width_ + bit_width > bit_field_base_type_width_ &&
          IsStruct()) {
        AddSpaceBetweenBitField();
        bit_field_base_type_width_ = new_bit_field_base_type_width;
      }

      if (bit_field_used_width_ == 0) {
        member->SetOffset(offset_);
        member->SetBitFieldBegin(0);

        member->GetIndexs().push_front({this, index_});

        members_.push_back(member);
        scope_->InsertUsual(member);

        if (!is_struct_) {
          UnionAddBitField(member->GetType());
        }
      } else {
        member->SetOffset(offset_);

        if (is_struct_) {
          member->SetBitFieldBegin(bit_field_used_width_);
        } else {
          member->SetBitFieldBegin(0);
        }

        member->GetIndexs().push_front({this, index_});

        members_.push_back(member);
        scope_->InsertUsual(member);

        if (!is_struct_) {
          UnionAddBitField(member->GetType());
        }
      }

      bit_field_used_width_ += member->GetBitFieldWidth();

      assert(bit_field_used_width_ <= bit_field_base_type_width_);
      if (bit_field_used_width_ == bit_field_base_type_width_) {
        BitFieldPacked();
      }
    }
  }
}

void StructType::Finish() {
  if (bit_field_used_width_ != 0) {
    align_ = std::max(align_, members_.back()->GetType()->GetAlign());
    AddBitFieldBeforeMember();
  }

  if (!is_struct_) {
    assert(std::size(llvm_types_) == 0 || std::size(llvm_types_) == 1);
  }

  auto struct_type{llvm::cast<llvm::StructType>(llvm_type_)};
  assert(struct_type->getStructNumElements() == 0);
  struct_type->setBody(llvm_types_);

  members_.erase(std::remove_if(std::begin(members_), std::end(members_),
                                [](ObjectExpr *obj) {
                                  return obj->IsAnonymous() &&
                                         obj->GetBitFieldWidth();
                                }),
                 std::end(members_));
}

std::int32_t StructType::MakeAlign(std::int32_t offset, std::int32_t align) {
  assert(offset >= 0);
  assert(align > 0);

  if (offset % align == 0) {
    return offset;
  } else {
    return offset + align - (offset % align);
  }
}

StructType::StructType(bool is_struct, const std::string &name, Scope *parent)
    : Type{false}, is_struct_{is_struct}, name_{name}, scope_{Scope::Get(
                                                           parent, kBlock)} {
  std::string prefix{is_struct ? "struct." : "union."};

  if (HasName()) {
    llvm_type_ = llvm::StructType::create(Context, prefix + name);
  } else {
    llvm_type_ = llvm::StructType::create(Context, prefix + "anon");
  }
}

void StructType::AddLLVMType(Type *type) {
  if (is_struct_) {
    llvm_types_.push_back(type->GetLLVMType());
  } else {
    assert(std::size(llvm_types_) == 0 || std::size(llvm_types_) == 1);

    if (std::empty(llvm_types_)) {
      llvm_types_.push_back(type->GetLLVMType());
    } else {
      auto &back{llvm_types_.back()};
      auto llvm_type{type->GetLLVMType()};

      if (GetLLVMTypeSize(back) < GetLLVMTypeSize(llvm_type)) {
        back = llvm_type;
      } else if (GetLLVMTypeSize(back) == GetLLVMTypeSize(llvm_type)) {
        if (back->isStructTy() && !llvm_type->isStructTy()) {
          back = llvm_type;
        }
      }
    }
  }
}

void StructType::AddBitFieldBeforeMember() {
  if (bit_field_used_width_ == 0) {
    bit_field_base_type_width_ = 0;
    return;
  }

  assert(bit_field_base_type_width_ != 0);

  if (is_struct_) {
    llvm_types_.push_back(GetBitFieldSpace(bit_field_used_width_));
    ++index_;

    offset_ += MakeAlign(bit_field_used_width_, 8) / 8;
    width_ = MakeAlign(offset_, align_);

    if (auto space_width{(width_ - offset_) * 8}) {
      auto space{GetBitFieldSpace(space_width)};
      llvm_types_.push_back(space);
      ++index_;
      bit_field_space_count_ += width_ - offset_;

      offset_ += GetLLVMTypeSize(space);
      width_ = MakeAlign(offset_, align_);
    }
  } else {
    auto type{GetBitFieldSpace(bit_field_used_width_)};
    if (std::empty(llvm_types_)) {
      llvm_types_.push_back(type);
    } else {
      if (GetLLVMTypeSize(llvm_types_.back()) < GetLLVMTypeSize(type)) {
        llvm_types_.back() = type;
      }
    }

    ++index_;

    width_ = std::max(width_, MakeAlign(bit_field_used_width_, 8) / 8);
    width_ = MakeAlign(width_, align_);
  }

  bit_field_used_width_ = 0;
  bit_field_base_type_width_ = 0;
}

void StructType::AddSpace(std::int32_t width) {
  if (auto space_width{width * 8}) {
    auto space{GetBitFieldSpace(space_width)};
    llvm_types_.push_back(space);
    ++index_;

    offset_ = offset_ + GetLLVMTypeSize(space);
    width_ = MakeAlign(offset_, align_);
  }
}

void StructType::UnionAddBitField(Type *type) {
  if (std::empty(llvm_types_)) {
    llvm_types_.push_back(type->GetLLVMType());
  } else {
    if (GetLLVMTypeSize(type->GetLLVMType()) >
        GetLLVMTypeSize(llvm_types_.back())) {
      llvm_types_.back() = type->GetLLVMType();
    }
  }

  assert(offset_ == 0);
  width_ = std::max(width_, bit_field_base_type_width_ / 8);
  width_ = MakeAlign(width_, align_);
}

void StructType::BitFieldPacked() {
  if (is_struct_) {
    llvm_types_.push_back(GetBitFieldSpace(bit_field_base_type_width_));

    offset_ += bit_field_base_type_width_ / 8;
    width_ = MakeAlign(offset_, align_);

    ++index_;
  }

  bit_field_used_width_ = 0;
  bit_field_base_type_width_ = 0;
}

void StructType::AddSpaceBetweenBitField() {
  assert(bit_field_base_type_width_ != 0);

  llvm_types_.push_back(GetBitFieldSpace(bit_field_used_width_));
  ++index_;

  offset_ += MakeAlign(bit_field_used_width_, 8) / 8;
  width_ = MakeAlign(offset_, align_);

  auto space{GetBitFieldSpace(bit_field_base_type_width_ -
                              MakeAlign(bit_field_used_width_, 8))};

  bit_field_used_width_ = 0;
  bit_field_base_type_width_ = 0;

  llvm_types_.push_back(space);
  ++index_;

  offset_ += GetLLVMTypeSize(space);
  width_ = MakeAlign(offset_, align_);
}

/*
 * FunctionType
 */
FunctionType *FunctionType::Get(QualType return_type,
                                std::vector<ObjectExpr *> params,
                                bool is_var_args) {
  return new (FunctionTypePool.malloc())
      FunctionType{return_type, params, is_var_args};
}

std::int32_t FunctionType::GetWidth() const {
  // GNU 扩展
  return 1;
}

std::int32_t FunctionType::GetAlign() const {
  assert(false);
  return 0;
}

// 其返回类型兼容
// 它们都使用参数列表, 参数数量(包括省略号的使用)相同, 而其对应参数,
// 在应用数组到指针和函数到指针类型调整, 及剥除顶层限定符后, 拥有相同类型
bool FunctionType::Compatible(const Type *other) const {
  if (other->IsFunctionTy()) {
    auto other_func{other->ToFunctionType()};
    if (!return_type_->Compatible(other_func->return_type_.GetType())) {
      return false;
    }
    if (GetNumParams() != other_func->GetNumParams()) {
      return false;
    }

    auto iter{std::begin(other_func->params_)};
    for (const auto &param : params_) {
      if (!param->GetType()->Equal((*iter)->GetType())) {
        return false;
      }
      ++iter;
    }

    return true;
  } else {
    return false;
  }
}

bool FunctionType::Equal(const Type *other) const {
  if (other->IsFunctionTy()) {
    auto other_func{other->ToFunctionType()};
    if (!return_type_->Equal(other_func->return_type_.GetType())) {
      return false;
    }
    if (GetNumParams() != other_func->GetNumParams()) {
      return false;
    }

    auto iter{std::begin(other_func->params_)};
    for (const auto &param : params_) {
      if (!param->GetType()->Equal((*iter)->GetType())) {
        return false;
      }
      ++iter;
    }

    return true;
  } else {
    return false;
  }
}

bool FunctionType::IsVarArgs() const { return is_var_args_; }

QualType FunctionType::GetReturnType() const { return return_type_; }

std::int32_t FunctionType::GetNumParams() const { return std::size(params_); }

std::vector<ObjectExpr *> &FunctionType::GetParams() { return params_; }

const std::vector<ObjectExpr *> &FunctionType::GetParams() const {
  return params_;
}

void FunctionType::SetFuncSpec(std::uint32_t func_spec) {
  func_spec_ = func_spec;
}

bool FunctionType::IsInline() const { return func_spec_ & kInline; }

void FunctionType::SetName(const std::string &name) { name_ = name; }

const std::string &FunctionType::GetName() const { return name_; }

FunctionType::FunctionType(QualType return_type,
                           std::vector<ObjectExpr *> param, bool is_var_args)
    : Type{false}, return_type_{return_type}, params_{param}, is_var_args_{
                                                                  is_var_args} {
  std::vector<llvm::Type *> params;
  for (const auto &item : params_) {
    params.push_back(item->GetType()->GetLLVMType());
  }

  llvm_type_ = llvm::FunctionType::get(return_type_->GetLLVMType(), params,
                                       is_var_args_);
}

} // namespace kcc
