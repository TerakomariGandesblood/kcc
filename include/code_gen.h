//
// Created by kaiser on 2019/11/2.
//

#pragma once

#include <cstdint>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/ValueHandle.h>

#include "ast.h"
#include "debug_info.h"
#include "visitor.h"

namespace kcc {

#define Load_Struct_Obj()                                                      \
  {                                                                            \
    auto backup{load_struct_};                                                 \
    load_struct_ = true;

#define Finish_Load()                                                          \
  load_struct_ = backup;                                                       \
  }

class CodeGen : public Visitor {
public:
  void GenCode(const TranslationUnit *root);

private:
  struct BreakContinue {
  public:
    BreakContinue(llvm::BasicBlock *break_block,
                  llvm::BasicBlock *continue_block);

    llvm::BasicBlock *break_block;
    llvm::BasicBlock *continue_block;
  };

  static llvm::BasicBlock *CreateBasicBlock(const std::string &name = "",
                                            llvm::Function *parent = nullptr);
  void EmitBlock(llvm::BasicBlock *bb, bool is_finished = false);
  static void EmitBranch(llvm::BasicBlock *target);
  bool HaveInsertPoint() const;
  void EnsureInsertPoint();
  llvm::Value *EvaluateExprAsBool(const Expr *expr);
  void EmitBranchOnBoolExpr(const Expr *expr, llvm::BasicBlock *true_block,
                            llvm::BasicBlock *false_block);
  static void SimplifyForwardingBlocks(llvm::BasicBlock *bb);
  void EmitStmt(const Stmt *stmt);
  bool EmitSimpleStmt(const Stmt *stmt);
  static bool ContainsLabel(const Stmt *stmt);
  void EmitBranchThroughCleanup(llvm::BasicBlock *dest);
  llvm::BasicBlock *GetBasicBlockForLabel(const LabelStmt *label);
  static bool IsCheapEnoughToEvaluateUnconditionally(const Expr *expr);
  llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Type *type, std::int32_t align,
                                           const std::string &name);
  llvm::Value *GetPtr(const AstNode *node);
  void PushBlock(llvm::BasicBlock *break_stack,
                 llvm::BasicBlock *continue_block);
  void PopBlock();
  bool TestAndClearIgnoreAssignResult();
  void SetIgnoreAssignResult();

  void TryEmitLocation(const AstNode *node);
  void TryEmitFuncStart(const FuncDef *node);
  void TryEmitFuncEnd();
  void TryEmitParamVar(const std::string &name, Type *type,
                       llvm::AllocaInst *ptr, const Location &loc);
  void TryEmitLocalVar(const Declaration *node);
  void TryEmitGlobalVar(const Declaration *node);

  virtual void Visit(const UnaryOpExpr *node) override;
  virtual void Visit(const TypeCastExpr *node) override;
  virtual void Visit(const BinaryOpExpr *node) override;
  virtual void Visit(const ConditionOpExpr *node) override;
  virtual void Visit(const FuncCallExpr *node) override;
  virtual void Visit(const ConstantExpr *node) override;
  virtual void Visit(const StringLiteralExpr *node) override;
  virtual void Visit(const IdentifierExpr *node) override;
  virtual void Visit(const EnumeratorExpr *node) override;
  virtual void Visit(const ObjectExpr *node) override;
  virtual void Visit(const StmtExpr *node) override;

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

  llvm::Value *IncOrDec(const Expr *expr, bool is_inc, bool is_postfix);
  static llvm::Value *NegOp(llvm::Value *value, bool is_unsigned);
  llvm::Value *LogicNotOp(llvm::Value *value);
  llvm::Value *Deref(const UnaryOpExpr *node);

  static llvm::Value *AddOp(llvm::Value *lhs, llvm::Value *rhs,
                            bool is_unsigned);
  static llvm::Value *SubOp(llvm::Value *lhs, llvm::Value *rhs,
                            bool is_unsigned);
  static llvm::Value *MulOp(llvm::Value *lhs, llvm::Value *rhs,
                            bool is_unsigned);
  static llvm::Value *DivOp(llvm::Value *lhs, llvm::Value *rhs,
                            bool is_unsigned);
  static llvm::Value *ModOp(llvm::Value *lhs, llvm::Value *rhs,
                            bool is_unsigned);
  static llvm::Value *OrOp(llvm::Value *lhs, llvm::Value *rhs);
  static llvm::Value *AndOp(llvm::Value *lhs, llvm::Value *rhs);
  static llvm::Value *XorOp(llvm::Value *lhs, llvm::Value *rhs);
  static llvm::Value *ShlOp(llvm::Value *lhs, llvm::Value *rhs);
  static llvm::Value *ShrOp(llvm::Value *lhs, llvm::Value *rhs,
                            bool is_unsigned);
  static llvm::Value *LessEqualOp(llvm::Value *lhs, llvm::Value *rhs,
                                  bool is_unsigned);
  static llvm::Value *LessOp(llvm::Value *lhs, llvm::Value *rhs,
                             bool is_unsigned);
  static llvm::Value *GreaterEqualOp(llvm::Value *lhs, llvm::Value *rhs,
                                     bool is_unsigned);
  static llvm::Value *GreaterOp(llvm::Value *lhs, llvm::Value *rhs,
                                bool is_unsigned);
  static llvm::Value *EqualOp(llvm::Value *lhs, llvm::Value *rhs);
  static llvm::Value *NotEqualOp(llvm::Value *lhs, llvm::Value *rhs);
  llvm::Value *LogicOrOp(const BinaryOpExpr *node);
  llvm::Value *LogicAndOp(const BinaryOpExpr *node);
  llvm::Value *AssignOp(const BinaryOpExpr *node);
  llvm::Value *MemberRef(const BinaryOpExpr *node);
  llvm::Value *Assign(llvm::Value *lhs_ptr, llvm::Value *rhs, bool is_unsigned);

  bool MayCallBuiltinFunc(const FuncCallExpr *node);
  llvm::Value *VaStart(Expr *arg);
  llvm::Value *VaEnd(Expr *arg);
  llvm::Value *VaArg(Expr *arg, llvm::Type *type);
  llvm::Value *VaCopy(Expr *arg, Expr *arg2);
  llvm::Value *SyncSynchronize();
  llvm::Value *Alloc(Expr *arg);
  llvm::Value *PopCount(Expr *arg);
  llvm::Value *Clz(Expr *arg);
  llvm::Value *Ctz(Expr *arg);
  llvm::Value *IsInfSign(Expr *arg);
  llvm::Value *IsFinite(Expr *arg);
  llvm::Value *Bswap16(Expr *arg);
  llvm::Value *Bswap32(Expr *arg);
  llvm::Value *Bswap64(Expr *arg);

  void DealLocaleDecl(const Declaration *node);
  void InitLocalAggregate(const Declaration *node);

  void StartFunction(const FuncDef *node);
  void FinishFunction(const FuncDef *node);
  void EmitFunctionEpilog();
  void EmitReturnBlock();

  llvm::Value *result_{};

  llvm::AssertingVH<llvm::Instruction> alloc_insert_point_;
  bool load_struct_{false};

  std::stack<BreakContinue> break_continue_stack_;
  std::unordered_map<const LabelStmt *, llvm::BasicBlock *> labels_;
  llvm::SwitchInst *switch_inst_{};

  llvm::Function *func_{};
  llvm::BasicBlock *return_block_{};
  llvm::Value *return_value_{};

  bool is_bit_field_{false};
  ObjectExpr *bit_field_{nullptr};

  bool is_volatile_{false};
  bool ignore_assign_result_{false};

  std::unique_ptr<DebugInfo> debug_info_;
};

} // namespace kcc
