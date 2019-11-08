//
// Created by kaiser on 2019/10/31.
//

#ifndef KCC_SRC_TYPE_H_
#define KCC_SRC_TYPE_H_

#include <llvm/IR/Type.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

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
  kRestrict = 0x2,
  // 不支持
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
class PointerType;
class Object;
class Scope;

class QualType {
  friend bool operator==(QualType lhs, QualType rhs);
  friend bool operator!=(QualType lhs, QualType rhs);

 public:
  QualType() = default;
  QualType(Type* type, std::uint32_t type_qual = 0)
      : type_{type}, type_qual_{type_qual} {}

  Type& operator*();
  const Type& operator*() const;
  Type* operator->();
  const Type* operator->() const;

  Type* GetType();
  const Type* GetType() const;
  std::uint32_t GetTypeQual() const;

  bool IsConst() const;
  bool IsRestrict() const;

 private:
  Type* type_;
  std::uint32_t type_qual_;
};

bool operator==(QualType lhs, QualType rhs);
bool operator!=(QualType lhs, QualType rhs);

class Type {
 public:
  // 数组函数隐式转换为指针
  // TODO 是否应该创建一个 ast 节点？
  static QualType MayCast(QualType type, bool in_proto = false);

  virtual ~Type() = default;

  // 字节数
  virtual std::int32_t GetWidth() const = 0;
  virtual std::int32_t GetAlign() const = 0;
  // 这里忽略了 cvr
  // 若涉及同一对象或函数的二个声明不使用兼容类型，则程序的行为未定义。
  virtual bool Compatible(const Type* other) const = 0;
  virtual bool Equal(const Type* other) const = 0;

  std::string ToString() const;
  llvm::Type* GetLLVMType() const;

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
  bool IsComplexTy() const;
  bool IsTypeName() const;

  bool IsPointerTy() const;
  bool IsArrayTy() const;
  bool IsStructTy() const;
  bool IsUnionTy() const;
  bool IsFunctionTy() const;

  bool IsObjectTy() const;
  bool IsCharacterTy() const;
  bool IsIntegerTy() const;
  bool IsRealTy() const;
  bool IsArithmeticTy() const;
  bool IsScalarTy() const;
  bool IsAggregateTy() const;
  bool IsDerivedTy() const;

  bool IsRealFloatPointTy() const;
  bool IsFloatPointTy() const;

  PointerType* GetPointerTo();

  std::int32_t ArithmeticRank() const;
  std::uint64_t ArithmeticMaxIntegerValue() const;
  void ArithmeticSetUnsigned();

  QualType PointerGetElementType() const;

  void ArraySetNumElements(std::size_t num_elements);
  std::size_t ArrayGetNumElements() const;
  QualType ArrayGetElementType() const;

  bool StructHasName() const;
  void StructSetName(const std::string& name);
  std::string StructGetName() const;
  std::int32_t StructGetNumMembers() const;
  std::vector<Object*> StructGetMembers();
  Object* StructGetMember(const std::string& name) const;
  QualType StructGetMemberType(std::int32_t i) const;
  std::shared_ptr<Scope> StructGetScope();
  void StructAddMember(Object* member);
  void StructMergeAnonymous(Object* anonymous);
  std::int32_t StructGetOffset() const;
  void StructFinish();

  bool FuncIsVarArgs() const;
  QualType FuncGetReturnType() const;
  std::int32_t FuncGetNumParams() const;
  QualType FuncGetParamType(std::int32_t i) const;
  std::vector<Object*> FuncGetParams() const;
  void FuncSetFuncSpec(std::uint32_t func_spec);
  bool FuncIsInline() const;
  bool FuncIsNoreturn() const;

 protected:
  explicit Type(bool complete);
  llvm::Type* llvm_type_{};

 private:
  mutable bool complete_{false};
};

class VoidType : public Type {
 public:
  static VoidType* Get();

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual bool Compatible(const Type* other) const override;
  virtual bool Equal(const Type* other) const override;

 private:
  VoidType();
};

class ArithmeticType : public Type {
  friend class Type;

 public:
  static ArithmeticType* Get(std::uint32_t type_spec);

  static Type* IntegerPromote(Type* type);
  static Type* MaxType(Type* lhs, Type* rhs);

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual bool Compatible(const Type* other) const override;
  virtual bool Equal(const Type* other) const override;

  std::int32_t Rank() const;
  std::uint64_t MaxIntegerValue() const;
  void SetUnsigned();

 private:
  explicit ArithmeticType(std::uint32_t type_spec);

  std::uint32_t type_spec_{};
};

class PointerType : public Type {
 public:
  static PointerType* Get(QualType element_type);

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual bool Compatible(const Type* other) const override;
  virtual bool Equal(const Type* type) const override;

  QualType GetElementType() const;

 private:
  explicit PointerType(QualType element_type);

  QualType element_type_;
};

class ArrayType : public Type {
 public:
  static ArrayType* Get(QualType contained_type, std::size_t num_elements = 0);

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual bool Compatible(const Type* other) const override;
  virtual bool Equal(const Type* other) const override;

  void SetNumElements(std::size_t num_elements);
  std::size_t GetNumElements() const;
  QualType GetElementType() const;

 private:
  ArrayType(QualType contained_type, std::size_t num_elements);

  QualType contained_type_;
  std::size_t num_elements_;
};

class StructType : public Type {
  friend class Type;

 public:
  static StructType* Get(bool is_struct, const std::string& name,
                         std::shared_ptr<Scope> parent);

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual bool Compatible(const Type* other) const override;
  virtual bool Equal(const Type* other) const override;

  bool IsStruct() const;
  bool HasName() const;
  void SetName(const std::string& name);
  std::string GetName() const;

  std::int32_t GetNumMembers() const;
  std::vector<Object*> GetMembers();
  Object* GetMember(const std::string& name) const;
  QualType GetMemberType(std::int32_t i) const;
  std::shared_ptr<Scope> GetScope();
  std::int32_t GetOffset() const;

  void AddMember(Object* member);
  void MergeAnonymous(Object* anonymous);
  void Finish();

 private:
  StructType(bool is_struct, const std::string& name,
             std::shared_ptr<Scope> parent);

  // 计算新成员的开始位置
  static std::int32_t MakeAlign(std::int32_t offset, std::int32_t align);

  bool is_struct_{};
  std::string name_;
  std::vector<Object*> members_;
  std::shared_ptr<Scope> scope_;

  std::int32_t offset_{};
  std::int32_t width_{};
  std::int32_t align_{};
};

class FunctionType : public Type {
 public:
  static FunctionType* Get(QualType return_type, std::vector<Object*> params,
                           bool is_var_args = false);

  virtual std::int32_t GetWidth() const override;
  virtual std::int32_t GetAlign() const override;
  virtual bool Compatible(const Type* other) const override;
  virtual bool Equal(const Type* other) const override;

  bool IsVarArgs() const;
  QualType GetReturnType() const;
  std::int32_t GetNumParams() const;
  QualType GetParamType(std::int32_t i) const;
  std::vector<Object*> GetParams() const;
  void SetFuncSpec(std::uint32_t func_spec);
  bool IsInline() const;
  bool IsNoreturn() const;

 private:
  FunctionType(QualType return_type, std::vector<Object*> param,
               bool is_var_args);

  bool is_var_args_;
  std::uint32_t func_spec_{};
  QualType return_type_;
  std::vector<Object*> params_;
};

}  // namespace kcc

#endif  // KCC_SRC_TYPE_H_
