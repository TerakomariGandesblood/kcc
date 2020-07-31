//
// Created by kaiser on 2019/10/31.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <llvm/IR/Type.h>

namespace kcc {

enum TypeSpec {
  kSigned = 0x1,
  kUnsigned = 0x2,
  kVoid = 0x4,
  kChar = 0x8,
  kShort = 0x10,
  kInt = 0x20,
  kLong = 0x40,
  kFloat = 0x80,
  kDouble = 0x100,
  kBool = 0x200,
  // 不支持
  kComplex = 0x400,
  // 不支持
  kAtomicTypeSpec = 0x800,
  kStructUnionSpec = 0x1000,
  kEnumSpec = 0x2000,
  kTypedefName = 0x4000,

  kLongLong = 0x8000
};

enum TypeQualifier {
  kConst = 0x1,
  // 不支持
  kRestrict = 0x2,
  kVolatile = 0x4,
  // 不支持
  kAtomic = 0x8
};

enum StorageClassSpec {
  kTypedef = 0x1,
  kExtern = 0x2,
  kStatic = 0x4,
  // 不支持
  kThreadLocal = 0x8,
  kAuto = 0x10,
  kRegister = 0x20
};

enum FuncSpec { kInline = 0x1, kNoreturn = 0x2 };

enum TypeSpecCompatibility {
  kCompSigned = kShort | kInt | kLong | kLongLong,
  kCompUnsigned = kShort | kInt | kLong | kLongLong,
  kCompChar = kSigned | kUnsigned,
  kCompShort = kSigned | kUnsigned | kInt,
  kCompInt = kSigned | kUnsigned | kShort | kLong | kLongLong,
  kCompLong = kSigned | kUnsigned | kInt | kLong,
  kCompDouble = kLong
};

class Type;
class VoidType;
class ArithmeticType;
class PointerType;
class ArrayType;
class StructType;
class FunctionType;
class ObjectExpr;
class Scope;

class QualType {
  friend bool operator==(QualType lhs, QualType rhs);
  friend bool operator!=(QualType lhs, QualType rhs);

public:
  QualType() = default;
  QualType(Type *type, std::uint32_t type_qual = 0);

  std::string ToString() const;

  Type *operator->();
  const Type *operator->() const;

  Type *GetType();
  const Type *GetType() const;

  std::uint32_t GetTypeQual() const;

  bool IsConst() const;
  bool IsVolatile() const;

private:
  Type *type_{};
  std::uint32_t type_qual_{};
};

bool operator==(QualType lhs, QualType rhs);
bool operator!=(QualType lhs, QualType rhs);

class Type {
public:
  // 数组和函数隐式转换为指针
  static QualType MayCast(QualType type);

  virtual ~Type() = default;

  // 字节数
  virtual std::int32_t GetWidth() const = 0;
  virtual std::int32_t GetAlign() const = 0;
  // 这里忽略了 cvr
  // 若涉及同一对象或函数的二个声明不使用兼容类型, 则程序的行为未定义
  virtual bool Compatible(const Type *other) const = 0;
  virtual bool Equal(const Type *other) const = 0;

  std::string ToString() const;
  llvm::Type *GetLLVMType() const;

  VoidType *ToVoidType();
  ArithmeticType *ToArithmeticType();
  PointerType *ToPointerType();
  ArrayType *ToArrayType();
  StructType *ToStructType();
  FunctionType *ToFunctionType();

  const VoidType *ToVoidType() const;
  const ArithmeticType *ToArithmeticType() const;
  const PointerType *ToPointerType() const;
  const ArrayType *ToArrayType() const;
  const StructType *ToStructType() const;
  const FunctionType *ToFunctionType() const;

  bool IsComplete() const;
  void SetComplete(bool complete);

  bool IsUnsigned() const;
  bool IsVoidTy() const;
  bool IsBoolTy() const;
  bool IsShortTy() const;
  bool IsIntTy() const;
  bool IsLongTy() const;
  bool IsLongLongTy() const;
  bool IsFloatTy() const;
  bool IsDoubleTy() const;
  bool IsLongDoubleTy() const;

  bool IsPointerTy() const;
  bool IsArrayTy() const;
  bool IsStructTy() const;
  bool IsUnionTy() const;
  bool IsStructOrUnionTy() const;
  bool IsFunctionTy() const;

  bool IsCharacterTy() const;
  bool IsIntegerTy() const;
  bool IsRealTy() const;
  bool IsArithmeticTy() const;
  bool IsScalarTy() const;
  bool IsAggregateTy() const;

  bool IsRealFloatPointTy() const;
  bool IsFloatPointTy() const;

  bool IsIntegerOrBoolTy() const;

  PointerType *GetPointerTo();

  std::int32_t ArithmeticRank() const;
  std::uint64_t ArithmeticMaxIntegerValue() const;

  QualType PointerGetElementType() const;

  void ArraySetNumElements(std::size_t num_elements);
  std::size_t ArrayGetNumElements() const;
  QualType ArrayGetElementType() const;

  bool StructHasName() const;
  void StructSetName(const std::string &name);
  const std::string &StructGetName() const;
  std::vector<ObjectExpr *> &StructGetMembers();
  const std::vector<ObjectExpr *> &StructGetMembers() const;
  void StructSetMembers(std::vector<ObjectExpr *> &members);
  ObjectExpr *StructGetMember(const std::string &name) const;
  QualType StructGetMemberType(std::int32_t i) const;
  Scope *StructGetScope();
  void StructAddMember(ObjectExpr *member);
  void StructMergeAnonymous(ObjectExpr *anonymous);
  std::int32_t StructGetOffset() const;
  void StructFinish();

  bool FuncIsVarArgs() const;
  QualType FuncGetReturnType() const;
  std::vector<ObjectExpr *> &FuncGetParams();
  const std::vector<ObjectExpr *> &FuncGetParams() const;
  void FuncSetFuncSpec(std::uint32_t func_spec);
  bool FuncIsInline() const;
  void FuncSetName(const std::string &name);
  const std::string &FuncGetName() const;

protected:
  explicit Type(bool complete);

  llvm::Type *llvm_type_{};

private:
  mutable bool complete_{false};
};

class VoidType : public Type {
public:
  static VoidType *Get();

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual bool Compatible(const Type *other) const override;
  virtual bool Equal(const Type *other) const override;

private:
  VoidType();
};

class ArithmeticType : public Type {
  friend class Type;

public:
  static ArithmeticType *Get(std::uint32_t type_spec);

  static Type *IntegerPromote(Type *type);
  static Type *MaxType(Type *lhs, Type *rhs);

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual bool Compatible(const Type *other) const override;
  virtual bool Equal(const Type *other) const override;

private:
  explicit ArithmeticType(std::uint32_t type_spec);

  std::int32_t Rank() const;
  std::uint64_t MaxIntegerValue() const;

  static std::uint32_t DealWithTypeSpec(std::uint32_t type_spec);

  std::uint32_t type_spec_{};
};

class PointerType : public Type {
public:
  static PointerType *Get(QualType element_type);

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual bool Compatible(const Type *other) const override;
  virtual bool Equal(const Type *type) const override;

  QualType GetElementType() const;

private:
  explicit PointerType(QualType element_type);

  QualType element_type_;
};

class ArrayType : public Type {
public:
  static ArrayType *Get(QualType contained_type,
                        std::optional<std::size_t> num_elements = {});

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual bool Compatible(const Type *other) const override;
  virtual bool Equal(const Type *other) const override;

  void SetNumElements(std::size_t num_elements);
  std::size_t GetNumElements() const;
  QualType GetElementType() const;

private:
  ArrayType(QualType contained_type, std::optional<std::size_t> num_elements);

  QualType contained_type_;
  std::optional<std::int64_t> num_elements_;
};

class StructType : public Type {
  friend class Type;

public:
  static StructType *Get(bool is_struct, const std::string &name,
                         Scope *parent);

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual bool Compatible(const Type *other) const override;
  virtual bool Equal(const Type *other) const override;

  bool IsStruct() const;
  bool HasName() const;
  void SetName(const std::string &name);
  const std::string &GetName() const;

  std::int32_t GetNumMembers() const;
  std::vector<ObjectExpr *> &GetMembers();
  const std::vector<ObjectExpr *> &GetMembers() const;
  void SetMembers(std::vector<ObjectExpr *> &members);
  ObjectExpr *GetMember(const std::string &name) const;
  QualType GetMemberType(std::int32_t i) const;
  Scope *GetScope();
  std::int32_t GetOffset() const;

  void AddMember(ObjectExpr *member);
  void MergeAnonymous(ObjectExpr *anonymous);
  void AddBitField(ObjectExpr *member);
  void Finish();

  // 计算新成员的开始位置
  static std::int32_t MakeAlign(std::int32_t offset, std::int32_t align);

private:
  StructType(bool is_struct, const std::string &name, Scope *parent);

  void AddLLVMType(Type *type);
  void AddBitFieldBeforeMember();
  void AddSpace(std::int32_t width);
  void UnionAddBitField(Type *type);
  void BitFieldPacked();
  void AddSpaceBetweenBitField();

  std::vector<llvm::Type *> llvm_types_;

  bool is_struct_{};
  std::string name_;
  std::vector<ObjectExpr *> members_;
  Scope *scope_{};

  std::int32_t offset_{};
  std::int32_t width_{};
  std::int32_t align_{1};

  std::int8_t bit_field_used_width_{};
  std::int8_t bit_field_base_type_width_{};

  std::int32_t index_{};

  std::int32_t bit_field_space_count_{};
};

class FunctionType : public Type {
public:
  static FunctionType *Get(QualType return_type,
                           std::vector<ObjectExpr *> params,
                           bool is_var_args = false);

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual bool Compatible(const Type *other) const override;
  virtual bool Equal(const Type *other) const override;

  bool IsVarArgs() const;
  QualType GetReturnType() const;
  std::int32_t GetNumParams() const;
  std::vector<ObjectExpr *> &GetParams();
  const std::vector<ObjectExpr *> &GetParams() const;
  void SetFuncSpec(std::uint32_t func_spec);
  bool IsInline() const;
  void SetName(const std::string &name);
  const std::string &GetName() const;

private:
  FunctionType(QualType return_type, std::vector<ObjectExpr *> param,
               bool is_var_args);

  QualType return_type_;
  std::vector<ObjectExpr *> params_;
  bool is_var_args_;

  std::uint32_t func_spec_{};

  std::string name_;
};

} // namespace kcc
