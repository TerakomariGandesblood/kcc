//
// Created by kaiser on 2019/10/31.
//

#pragma once

#include "ast.h"

namespace kcc {

class Visitor {
 public:
  virtual ~Visitor() = default;

  virtual void Visit(const UnaryOpExpr *node) = 0;
  virtual void Visit(const TypeCastExpr *node) = 0;
  virtual void Visit(const BinaryOpExpr *node) = 0;
  virtual void Visit(const ConditionOpExpr *node) = 0;
  virtual void Visit(const FuncCallExpr *node) = 0;
  virtual void Visit(const ConstantExpr *node) = 0;
  virtual void Visit(const StringLiteralExpr *node) = 0;
  virtual void Visit(const IdentifierExpr *node) = 0;
  virtual void Visit(const EnumeratorExpr *node) = 0;
  virtual void Visit(const ObjectExpr *node) = 0;
  virtual void Visit(const StmtExpr *node) = 0;

  virtual void Visit(const LabelStmt *node) = 0;
  virtual void Visit(const CaseStmt *node) = 0;
  virtual void Visit(const DefaultStmt *node) = 0;
  virtual void Visit(const CompoundStmt *node) = 0;
  virtual void Visit(const ExprStmt *node) = 0;
  virtual void Visit(const IfStmt *node) = 0;
  virtual void Visit(const SwitchStmt *node) = 0;
  virtual void Visit(const WhileStmt *node) = 0;
  virtual void Visit(const DoWhileStmt *node) = 0;
  virtual void Visit(const ForStmt *node) = 0;
  virtual void Visit(const GotoStmt *node) = 0;
  virtual void Visit(const ContinueStmt *node) = 0;
  virtual void Visit(const BreakStmt *node) = 0;
  virtual void Visit(const ReturnStmt *node) = 0;

  virtual void Visit(const TranslationUnit *node) = 0;
  virtual void Visit(const Declaration *node) = 0;
  virtual void Visit(const FuncDef *node) = 0;
};

}  // namespace kcc
