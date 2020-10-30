//
// Created by kaiser on 2019/10/31.
//

#pragma once

#include <cstdint>
#include <list>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APInt.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Instructions.h>

#include "location.h"
#include "token.h"
#include "type.h"

namespace kcc {

class ObjectExpr;
class CompoundStmt;
class Declaration;
class Visitor;

// arr / ptr
inline std::unordered_map<std::string,
                          std::pair<llvm::Constant *, llvm::Constant *>>
    StringMap;

inline std::unordered_map<std::string, llvm::GlobalVariable *> GlobalVarMap;

enum class AstNodeType {
  kUnaryOpExpr,
  kTypeCastExpr,
  kBinaryOpExpr,
  kConditionOpExpr,
  kFuncCallExpr,
  kConstantExpr,
  kStringLiteralExpr,
  kIdentifierExpr,
  kEnumeratorExpr,
  kObjectExpr,
  kStmtExpr,

  kLabelStmt,
  kCaseStmt,
  kDefaultStmt,
  kCompoundStmt,
  kExprStmt,
  kIfStmt,
  kSwitchStmt,
  kWhileStmt,
  kDoWhileStmt,
  kForStmt,
  kGotoStmt,
  kContinueStmt,
  kBreakStmt,
  kReturnStmt,

  kTranslationUnit,
  kDeclaration,
  kFuncDef,
};

class AstNode {
 public:
  virtual ~AstNode() = default;

  virtual AstNodeType Kind() const = 0;
  virtual void Accept(Visitor &visitor) const = 0;
  virtual void Check() = 0;

  std::string KindQString() const;

  const Location &GetLoc() const;
  void SetLoc(const Location &loc);

 protected:
  AstNode() = default;

  Location loc_;
};

class Expr : public AstNode {
 public:
  virtual bool IsLValue() const = 0;

  QualType GetQualType() const;
  Type *GetType();
  const Type *GetType() const;

  bool IsConst() const;
  bool IsVolatile() const;

  void EnsureCompatible(QualType lhs, QualType rhs) const;
  void EnsureCompatibleConvertVoidPtr(Expr *&lhs, Expr *&rhs);
  // 数组函数隐式转换
  static Expr *MayCast(Expr *expr);
  static Expr *MayCastTo(Expr *expr, QualType to);
  // 通常算术转换
  static Type *Convert(Expr *&lhs, Expr *&rhs);
  // 判断是否是 int 0
  static bool IsZero(const Expr *expr);

 protected:
  explicit Expr(QualType type = {});

  QualType type_;
};

/*
 * ++ --
 * + - ~
 * !
 * * &
 */
class UnaryOpExpr : public Expr {
 public:
  static UnaryOpExpr *Get(Tag tag, Expr *expr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

  Tag GetOp() const;
  const Expr *GetExpr() const;

 private:
  UnaryOpExpr(Tag tag, Expr *expr);

  void IncDecOpCheck();
  void UnaryAddSubOpCheck();
  void NotOpCheck();
  void LogicNotOpCheck();
  void DerefOpCheck();
  void AddrOpCheck();

  Tag op_;
  Expr *expr_;
};

class TypeCastExpr : public Expr {
 public:
  static TypeCastExpr *Get(Expr *expr, QualType to);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

  const Expr *GetExpr() const;
  QualType GetCastToType() const;

 private:
  TypeCastExpr(Expr *expr, QualType to);

  Expr *expr_;
};

/*
 * =
 * + - * / % & | ^ << >>
 * && ||
 * == != < > <= >=
 * .
 * ,
 */
// 复合赋值运算符, [] , -> 均做了转换
class BinaryOpExpr : public Expr {
 public:
  static BinaryOpExpr *Get(Tag tag, Expr *lhs, Expr *rhs);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

  Tag GetOp() const;
  const Expr *GetLHS() const;
  const Expr *GetRHS() const;

 private:
  BinaryOpExpr(Tag tag, Expr *lhs, Expr *rhs);

  void AssignOpCheck();
  void AddOpCheck();
  void SubOpCheck();
  void MultiOpCheck();
  void BitwiseOpCheck();
  void ShiftOpCheck();
  void LogicalOpCheck();
  void EqualityOpCheck();
  void RelationalOpCheck();
  void MemberRefOpCheck();
  void CommaOpCheck();

  Tag op_;
  Expr *lhs_;
  Expr *rhs_;
};

class ConditionOpExpr : public Expr {
 public:
  static ConditionOpExpr *Get(Expr *cond, Expr *lhs, Expr *rhs);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

  const Expr *GetCond() const;
  const Expr *GetLHS() const;
  const Expr *GetRHS() const;

 private:
  ConditionOpExpr(Expr *cond, Expr *lhs, Expr *rhs);

  Expr *cond_;
  Expr *lhs_;
  Expr *rhs_;
};

class FuncCallExpr : public Expr {
 public:
  static FuncCallExpr *Get(Expr *callee, std::vector<Expr *> args = {});

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

  Type *GetFuncType() const;

  Expr *GetCallee() const;
  const std::vector<Expr *> &GetArgs() const;

  void SetVaArgType(Type *va_arg_type);
  Type *GetVaArgType() const;

 private:
  explicit FuncCallExpr(Expr *callee, std::vector<Expr *> args = {});

  Expr *callee_;
  std::vector<Expr *> args_;

  Type *va_arg_type_{nullptr};
};

class ConstantExpr : public Expr {
 public:
  static ConstantExpr *Get(std::int32_t val);
  static ConstantExpr *Get(Type *type, std::uint64_t val);
  // for float point
  static ConstantExpr *Get(Type *type, const std::string &str);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

  const llvm::APInt &GetIntegerVal() const;
  const llvm::APFloat &GetFloatPointVal() const;

 private:
  ConstantExpr(std::int32_t val);
  ConstantExpr(Type *type, std::uint64_t val);
  ConstantExpr(Type *type, const std::string &str);

  llvm::APInt integer_val_;
  llvm::APFloat float_point_val_;
};

class StringLiteralExpr : public Expr {
 public:
  static StringLiteralExpr *Get(const std::string &val);
  static StringLiteralExpr *Get(Type *type, const std::string &val);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

  const std::string &GetStr() const;
  llvm::Constant *GetArr() const;
  llvm::Constant *GetPtr() const;

 private:
  StringLiteralExpr(Type *type, const std::string &val);

  std::pair<llvm::Constant *, llvm::Constant *> Create() const;

  std::string str_;
};

enum class Linkage { kNone, kInternal, kExternal };

// 标识符能指代下列类型的实体：
// 对象
// 函数
// struct / union / enum tag
// 结构体或联合体成员
// 枚举常量
// typedef 名
// 标号名
// 宏名
// 宏形参名
// 宏名或宏形参名以外的每个标识符都拥有作用域，并且可以拥有链接
class IdentifierExpr : public Expr {
 public:
  static IdentifierExpr *Get(const std::string &name, QualType type,
                             enum Linkage linkage = Linkage::kNone,
                             bool is_type_name = false);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

  enum Linkage GetLinkage() const;
  const std::string &GetName() const;
  bool IsTypeName() const;
  bool IsObject() const;

  ObjectExpr *ToObjectExpr();
  const ObjectExpr *ToObjectExpr() const;

 protected:
  IdentifierExpr(const std::string &name, QualType type,
                 enum Linkage linkage = Linkage::kNone,
                 bool is_type_name = false);

  std::string name_;
  enum Linkage linkage_;
  bool is_type_name_;
};

class EnumeratorExpr : public IdentifierExpr {
 public:
  static EnumeratorExpr *Get(const std::string &name, std::int32_t val);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

  std::int32_t GetVal() const;

 private:
  EnumeratorExpr(const std::string &name, std::int32_t val);

  std::int32_t val_;
};

// C 中, 一个对象是执行环境中数据存储的一个区域, 其内容可以表示值
// 每个对象拥有
// 大小
// 对齐要求
// 存储期
// 生存期(等于存储期或临时)
// 有效类型
// 值
// 可选项: 表示该对象的标识符
class ObjectExpr : public IdentifierExpr {
 public:
  static ObjectExpr *Get(const std::string &name, QualType type,
                         std::uint32_t storage_class_spec = 0,
                         enum Linkage linkage = Linkage::kNone,
                         bool anonymous = false,
                         std::int32_t bit_field_width = 0);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual bool IsLValue() const override;
  virtual void Check() override;

  bool IsStatic() const;
  bool IsExtern() const;
  void SetStorageClassSpec(std::uint32_t storage_class_spec);
  std::uint32_t GetStorageClassSpec();

  std::int32_t GetAlign() const;
  void SetAlign(std::int32_t align);
  std::int32_t GetOffset() const;
  void SetOffset(std::int32_t offset);

  Declaration *GetDecl() const;
  void SetDecl(Declaration *decl);

  bool IsAnonymous() const;
  bool IsGlobalVar() const;
  bool IsLocalStaticVar() const;

  void SetLocalPtr(llvm::AllocaInst *local_ptr);
  llvm::AllocaInst *GetLocalPtr() const;
  llvm::GlobalVariable *GetGlobalPtr() const;

  std::list<std::pair<Type *, std::int32_t>> &GetIndexs();
  const std::list<std::pair<Type *, std::int32_t>> &GetIndexs() const;

  std::int32_t GetBitFieldWidth() const;
  std::int32_t GetBitFieldBegin() const;
  void SetBitFieldBegin(std::int32_t bit_field_begin);

  void SetType(Type *type);

  void SetFuncName(const std::string &func_name);

 private:
  ObjectExpr(const std::string &name, QualType type,
             std::uint32_t storage_class_spec = 0,
             enum Linkage linkage = Linkage::kNone, bool anonymous = false,
             std::int32_t bit_field_width = 0);

  bool anonymous_{};

  std::uint32_t storage_class_spec_{};
  std::int32_t align_{};
  std::int32_t offset_{};

  std::int32_t bit_field_width_{};
  std::int32_t bit_field_begin_{};

  // 当遇到重复声明时使用
  Declaration *decl_{};

  // 用于索引结构体或数组成员
  std::list<std::pair<Type *, std::int32_t>> indexs_;

  llvm::AllocaInst *local_ptr_{};

  std::string func_name_;
};

// GNU 扩展, 语句表达式, 它可以是常量表达式, 不是左值表达式
class StmtExpr : public Expr {
 public:
  static StmtExpr *Get(CompoundStmt *block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual bool IsLValue() const override;

  const CompoundStmt *GetBlock() const;

 private:
  StmtExpr(CompoundStmt *block);

  CompoundStmt *block_;
};

class Stmt : public AstNode {
 public:
  virtual std::vector<Stmt *> Children() const;
};

class LabelStmt : public Stmt {
 public:
  static LabelStmt *Get(const std::string &name, Stmt *stmt);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual std::vector<Stmt *> Children() const override;

  Stmt *GetStmt() const;
  const std::string &GetName() const;

 private:
  explicit LabelStmt(const std::string &name, Stmt *stmt);

  std::string name_;
  Stmt *stmt_;
};

class CaseStmt : public Stmt {
 public:
  static CaseStmt *Get(std::int64_t lhs, Stmt *stmt);
  static CaseStmt *Get(std::int64_t lhs, std::int64_t rhs, Stmt *stmt);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual std::vector<Stmt *> Children() const override;

  std::int64_t GetLHS() const;
  std::optional<std::int64_t> GetRHS() const;
  const Stmt *GetStmt() const;

 private:
  CaseStmt(std::int64_t lhs, Stmt *stmt);
  CaseStmt(std::int64_t lhs, std::int64_t rhs, Stmt *stmt);

  std::int64_t lhs_{};
  std::optional<std::int64_t> rhs_{};

  Stmt *stmt_;
};

class DefaultStmt : public Stmt {
 public:
  static DefaultStmt *Get(Stmt *stmt);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual std::vector<Stmt *> Children() const override;

  const Stmt *GetStmt() const;

 private:
  DefaultStmt(Stmt *stmt);

  Stmt *stmt_;
};

class CompoundStmt : public Stmt {
 public:
  static CompoundStmt *Get();
  static CompoundStmt *Get(std::vector<Stmt *> stmts);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual std::vector<Stmt *> Children() const override;

  const std::vector<Stmt *> &GetStmts() const;
  void AddStmt(Stmt *stmt);

 private:
  CompoundStmt() = default;
  explicit CompoundStmt(std::vector<Stmt *> stmts);

  std::vector<Stmt *> stmts_;
};

class ExprStmt : public Stmt {
 public:
  static ExprStmt *Get(Expr *expr = nullptr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;

  Expr *GetExpr() const;

 private:
  explicit ExprStmt(Expr *expr = nullptr);

  Expr *expr_;
};

class IfStmt : public Stmt {
 public:
  static IfStmt *Get(Expr *cond, Stmt *then_block, Stmt *else_block = nullptr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual std::vector<Stmt *> Children() const override;

  const Expr *GetCond() const;
  const Stmt *GetThen() const;
  const Stmt *GetElse() const;

 private:
  IfStmt(Expr *cond, Stmt *then_block, Stmt *else_block = nullptr);

  Expr *cond_;
  Stmt *then_block_;
  Stmt *else_block_;
};

class SwitchStmt : public Stmt {
 public:
  static SwitchStmt *Get(Expr *cond, Stmt *stmt);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual std::vector<Stmt *> Children() const override;

  const Expr *GetCond() const;
  const Stmt *GetStmt() const;

 private:
  SwitchStmt(Expr *cond, Stmt *stmt);

  Expr *cond_;
  Stmt *stmt_;
};

class WhileStmt : public Stmt {
 public:
  static WhileStmt *Get(Expr *cond, Stmt *block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual std::vector<Stmt *> Children() const override;

  const Expr *GetCond() const;
  const Stmt *GetBlock() const;

 private:
  WhileStmt(Expr *cond, Stmt *block);

  Expr *cond_;
  Stmt *block_;
};

class DoWhileStmt : public Stmt {
 public:
  static DoWhileStmt *Get(Expr *cond, Stmt *block);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual std::vector<Stmt *> Children() const override;

  const Expr *GetCond() const;
  const Stmt *GetBlock() const;

 private:
  DoWhileStmt(Expr *cond, Stmt *block);

  Expr *cond_;
  Stmt *block_;
};

class ForStmt : public Stmt {
 public:
  static ForStmt *Get(Expr *init, Expr *cond, Expr *inc, Stmt *block,
                      Stmt *decl);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;
  virtual std::vector<Stmt *> Children() const override;

  const Expr *GetInit() const;
  const Expr *GetCond() const;
  const Expr *GetInc() const;
  const Stmt *GetBlock() const;
  const Stmt *GetDecl() const;

 private:
  ForStmt(Expr *init, Expr *cond, Expr *inc, Stmt *block, Stmt *decl);

  Expr *init_, *cond_, *inc_;
  Stmt *block_;
  Stmt *decl_;
};

class GotoStmt : public Stmt {
 public:
  static GotoStmt *Get(const std::string &name);
  static GotoStmt *Get(LabelStmt *label);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;

  const LabelStmt *GetLabel() const;
  void SetLabel(LabelStmt *label);
  const std::string &GetName() const;

 private:
  explicit GotoStmt(const std::string &name);
  explicit GotoStmt(LabelStmt *ident);

  std::string name_;
  LabelStmt *label_{};
};

class ContinueStmt : public Stmt {
 public:
  static ContinueStmt *Get();

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;

 private:
  ContinueStmt() = default;
};

class BreakStmt : public Stmt {
 public:
  static BreakStmt *Get();

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;

 private:
  BreakStmt() = default;
};

class ReturnStmt : public Stmt {
 public:
  static ReturnStmt *Get(Expr *expr = nullptr);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;

  const Expr *GetExpr() const;

 private:
  explicit ReturnStmt(Expr *expr = nullptr);

  Expr *expr_;
};

using ExtDecl = AstNode;

class TranslationUnit : public AstNode {
 public:
  static TranslationUnit *Get();

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;

  void AddExtDecl(ExtDecl *ext_decl);
  const std::vector<ExtDecl *> &GetExtDecl() const;

 private:
  std::vector<ExtDecl *> ext_decls_;
};

class Initializer {
 public:
  Initializer(
      Type *type, Expr *expr,
      std::vector<std::tuple<Type *, std::int32_t, std::int32_t, std::int32_t>>
          indexs);

  Type *GetType() const;

  Expr *&GetExpr();
  const Expr *GetExpr() const;

  const std::vector<
      std::tuple<Type *, std::int32_t, std::int32_t, std::int32_t>>
      &GetIndexs() const;

 private:
  Type *type_;
  Expr *expr_;
  std::vector<std::tuple<Type *, std::int32_t, std::int32_t, std::int32_t>>
      indexs_;
};

class Declaration : public Stmt {
 public:
  static Declaration *Get(IdentifierExpr *ident);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;

  void AddInits(std::vector<Initializer> inits);
  const std::vector<Initializer> &GetLocalInits() const;

  void SetConstant(llvm::Constant *constant);
  llvm::Constant *GetConstant() const;

  bool ValueInit() const;

  bool HasLocalInit() const;
  bool HasConstantInit() const;

  IdentifierExpr *GetIdent() const;

  bool IsObjDecl() const;
  ObjectExpr *GetObject() const;
  bool IsObjDeclInGlobalOrLocalStatic() const;

 private:
  explicit Declaration(IdentifierExpr *ident);

  IdentifierExpr *ident_;

  std::vector<Initializer> inits_;
  llvm::Constant *constant_{};

  bool value_init_{false};
};

class FuncDef : public ExtDecl {
 public:
  static FuncDef *Get(IdentifierExpr *ident);

  virtual AstNodeType Kind() const override;
  virtual void Accept(Visitor &visitor) const override;
  virtual void Check() override;

  void SetBody(CompoundStmt *body);

  const std::string &GetName() const;
  enum Linkage GetLinkage() const;
  Type *GetFuncType() const;
  IdentifierExpr *GetIdent() const;
  const CompoundStmt *GetBody() const;

 private:
  explicit FuncDef(IdentifierExpr *ident);

  IdentifierExpr *ident_;
  CompoundStmt *body_{};
};

template <typename T, typename... Args>
T *MakeAstNode(const Location &loc, Args &&...args) {
  auto t{T::Get(std::forward<Args>(args)...)};
  t->SetLoc(loc);
  t->Check();
  return t;
}

template <typename T, typename... Args>
T *MakeAstNode(const Token &token, Args &&...args) {
  return MakeAstNode<T>(token.GetLoc(), std::forward<Args>(args)...);
}

}  // namespace kcc
