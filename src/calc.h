//
// Created by kaiser on 2019/11/2.
//

#pragma once

#include <cstdint>
#include <optional>

#include <llvm/IR/Constants.h>

#include "ast.h"
#include "location.h"
#include "visitor.h"

namespace kcc {

class CalcConstantExpr : public Visitor {
public:
  explicit CalcConstantExpr(const Location &loc = {});

  llvm::Constant *Calc(const Expr *expr);
  std::optional<std::int64_t> CalcInteger(const Expr *expr,
                                          bool as_error = true);

private:
  static llvm::Constant *Throw(llvm::Constant *value = nullptr);

  virtual void Visit(const UnaryOpExpr *node) override;
  virtual void Visit(const TypeCastExpr *node) override;
  virtual void Visit(const BinaryOpExpr *node) override;
  virtual void Visit(const ConditionOpExpr *node) override;
  virtual void Visit(const ConstantExpr *node) override;
  virtual void Visit(const EnumeratorExpr *node) override;
  virtual void Visit(const StmtExpr *node) override;
  virtual void Visit(const StringLiteralExpr *node) override;

  virtual void Visit(const FuncCallExpr *node) override;
  virtual void Visit(const IdentifierExpr *node) override;
  virtual void Visit(const ObjectExpr *node) override;

  virtual void Visit(const LabelStmt *node) override;
  virtual void Visit(const CaseStmt *node) override;
  virtual void Visit(const DefaultStmt *node) override;
  virtual void Visit(const CompoundStmt *node) override;
  virtual void Visit(const ExprStmt *node) override;
  virtual void Visit(const IfStmt *node) override;
  virtual void Visit(const SwitchStmt *node) override;
  virtual void Visit(const WhileStmt *node) override;
  virtual void Visit(const DoWhileStmt *node) override;
  virtual void Visit(const ForStmt *node) override;
  virtual void Visit(const GotoStmt *node) override;
  virtual void Visit(const ContinueStmt *node) override;
  virtual void Visit(const BreakStmt *node) override;
  virtual void Visit(const ReturnStmt *node) override;

  virtual void Visit(const TranslationUnit *node) override;
  virtual void Visit(const Declaration *node) override;
  virtual void Visit(const FuncDef *node) override;

  static llvm::Constant *NegOp(llvm::Constant *value, bool is_unsigned);
  static llvm::Constant *LogicNotOp(llvm::Constant *value);
  static llvm::Constant *Addr(const UnaryOpExpr *node);

  static llvm::Constant *AddOp(llvm::Constant *lhs, llvm::Constant *rhs,
                               bool is_unsigned);
  static llvm::Constant *SubOp(llvm::Constant *lhs, llvm::Constant *rhs,
                               bool is_unsigned);
  static llvm::Constant *MulOp(llvm::Constant *lhs, llvm::Constant *rhs,
                               bool is_unsigned);
  static llvm::Constant *DivOp(llvm::Constant *lhs, llvm::Constant *rhs,
                               bool is_unsigned);
  static llvm::Constant *ModOp(llvm::Constant *lhs, llvm::Constant *rhs,
                               bool is_unsigned);
  static llvm::Constant *OrOp(llvm::Constant *lhs, llvm::Constant *rhs);
  static llvm::Constant *AndOp(llvm::Constant *lhs, llvm::Constant *rhs);
  static llvm::Constant *XorOp(llvm::Constant *lhs, llvm::Constant *rhs);
  static llvm::Constant *ShlOp(llvm::Constant *lhs, llvm::Constant *rhs);
  static llvm::Constant *ShrOp(llvm::Constant *lhs, llvm::Constant *rhs,
                               bool is_unsigned);
  static llvm::Constant *LessEqualOp(llvm::Constant *lhs, llvm::Constant *rhs,
                                     bool is_unsigned);
  static llvm::Constant *LessOp(llvm::Constant *lhs, llvm::Constant *rhs,
                                bool is_unsigned);
  static llvm::Constant *GreaterEqualOp(llvm::Constant *lhs,
                                        llvm::Constant *rhs, bool is_unsigned);
  static llvm::Constant *GreaterOp(llvm::Constant *lhs, llvm::Constant *rhs,
                                   bool is_unsigned);
  static llvm::Constant *EqualOp(llvm::Constant *lhs, llvm::Constant *rhs);
  static llvm::Constant *NotEqualOp(llvm::Constant *lhs, llvm::Constant *rhs);
  static llvm::Constant *LogicOrOp(const BinaryOpExpr *node);
  static llvm::Constant *LogicAndOp(const BinaryOpExpr *node);

  llvm::Constant *val_{};
  Location loc_;
};

} // namespace kcc
