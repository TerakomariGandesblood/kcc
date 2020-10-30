//
// Created by kaiser on 2019/12/13.
//

#include "code_gen.h"

#include <algorithm>
#include <cassert>

#include "calc.h"
#include "error.h"
#include "llvm_common.h"

namespace kcc {

void CodeGen::Visit(const LabelStmt *node) {
  TryEmitLocation(node);

  EmitBlock(GetBasicBlockForLabel(node));
  EmitStmt(node->GetStmt());
}

void CodeGen::Visit(const CaseStmt *node) {
  TryEmitLocation(node);

  auto block{CreateBasicBlock("switch.case")};
  EmitBlock(block);
  std::int64_t begin, end;

  if (auto rhs{node->GetRHS()}) {
    begin = node->GetLHS();
    end = *rhs;
  } else {
    begin = node->GetLHS();
    end = begin;
  }

  for (auto i{begin}; i <= end; ++i) {
    switch_inst_->addCase(llvm::ConstantInt::get(Builder.getInt64Ty(), i),
                          block);
  }

  EmitStmt(node->GetStmt());
}

void CodeGen::Visit(const DefaultStmt *node) {
  TryEmitLocation(node);

  auto default_block{switch_inst_->getDefaultDest()};
  assert(default_block->empty());

  EmitBlock(default_block);
  EmitStmt(node->GetStmt());
}

void CodeGen::Visit(const CompoundStmt *node) {
  for (const auto &item : node->GetStmts()) {
    EmitStmt(item);
  }
}

void CodeGen::Visit(const ExprStmt *node) {
  TryEmitLocation(node);

  if (auto expr{node->GetExpr()}) {
    expr->Accept(*this);
  } else {
    return;
  }
}

void CodeGen::Visit(const IfStmt *node) {
  TryEmitLocation(node);

  if (auto cond{CalcConstantExpr{}.Calc(node->GetCond())}) {
    auto executed{node->GetThen()}, skipped{node->GetElse()};
    if (cond->isZeroValue()) {
      std::swap(executed, skipped);
    }

    // 如果 skipped block 不包含 label, 那么可以不生成它
    if (!ContainsLabel(skipped)) {
      if (executed) {
        EmitStmt(executed);
      }
      return;
    }
  }

  auto then_block{CreateBasicBlock("if.then")};
  auto end_block{CreateBasicBlock("if.end")};
  auto else_block{end_block};

  if (node->GetElse()) {
    else_block = CreateBasicBlock("if.else");
  }

  EmitBranchOnBoolExpr(node->GetCond(), then_block, else_block);

  EmitBlock(then_block);
  EmitStmt(node->GetThen());
  EmitBranch(end_block);

  if (auto else_stmt{node->GetElse()}) {
    EmitBlock(else_block);
    EmitStmt(else_stmt);
    EmitBranch(end_block);
  }

  EmitBlock(end_block, true);
}

void CodeGen::Visit(const SwitchStmt *node) {
  TryEmitLocation(node);

  node->GetCond()->Accept(*this);
  auto cond_val{result_};

  auto switch_inst_backup{switch_inst_};

  auto default_block{CreateBasicBlock("switch.default")};
  auto end_block{CreateBasicBlock("switch.end")};
  switch_inst_ = Builder.CreateSwitch(cond_val, default_block);

  Builder.ClearInsertionPoint();

  llvm::BasicBlock *continue_block{};
  if (!std::empty(break_continue_stack_)) {
    continue_block = break_continue_stack_.top().continue_block;
  }
  PushBlock(end_block, continue_block);
  EmitStmt(node->GetStmt());
  PopBlock();

  if (!default_block->getParent()) {
    default_block->replaceAllUsesWith(end_block);
    delete default_block;
  }

  EmitBlock(end_block, true);

  switch_inst_ = switch_inst_backup;
}

void CodeGen::Visit(const WhileStmt *node) {
  TryEmitLocation(node);

  auto cond_block{CreateBasicBlock("while.cond")};
  EmitBlock(cond_block);

  auto body_block{CreateBasicBlock("while.body")};
  auto end_block{CreateBasicBlock("while.end")};

  PushBlock(end_block, cond_block);

  auto cond_val{EvaluateExprAsBool(node->GetCond())};

  // while(1)
  bool emit_br{true};
  if (auto constant{llvm::dyn_cast<llvm::ConstantInt>(cond_val)}) {
    if (constant->isOne()) {
      emit_br = false;
    }
  }
  if (emit_br) {
    Builder.CreateCondBr(cond_val, body_block, end_block);
  }

  EmitBlock(body_block);
  EmitStmt(node->GetBlock());
  PopBlock();

  EmitBranch(cond_block);

  EmitBlock(end_block, true);

  //  br label %while.cond
  //
  // while.cond:               ; preds = %while.body, %entry
  //  br label %while.body
  //
  // while.body:               ; preds = %while.cond
  //  br label %while.cond
  if (!emit_br) {
    SimplifyForwardingBlocks(cond_block);
  }
}

void CodeGen::Visit(const DoWhileStmt *node) {
  TryEmitLocation(node);

  auto body_block{CreateBasicBlock("do.while.body")};
  auto cond_block{CreateBasicBlock("do.while.cond")};
  auto end_block{CreateBasicBlock("do.while.end")};

  EmitBlock(body_block);
  PushBlock(end_block, cond_block);
  EmitStmt(node->GetBlock());
  PopBlock();

  EmitBlock(cond_block);
  auto cond_val{EvaluateExprAsBool(node->GetCond())};

  // do{}while(0)
  bool emit_br{true};
  if (auto constant{llvm::dyn_cast<llvm::ConstantInt>(cond_val)}) {
    if (constant->isZero()) {
      emit_br = false;
    }
  }
  if (emit_br) {
    Builder.CreateCondBr(cond_val, body_block, end_block);
  }

  EmitBlock(end_block);

  if (!emit_br) {
    SimplifyForwardingBlocks(cond_block);
  }
}

void CodeGen::Visit(const ForStmt *node) {
  TryEmitLocation(node);

  if (auto init{node->GetInit()}) {
    init->Accept(*this);
  } else if (auto decl{node->GetDecl()}) {
    EmitStmt(decl);
  }

  auto cond_block{CreateBasicBlock("for.cond")};
  auto end_block{CreateBasicBlock("for.end")};

  EmitBlock(cond_block);

  if (node->GetCond()) {
    auto body_block{CreateBasicBlock("for.body")};
    EmitBranchOnBoolExpr(node->GetCond(), body_block, end_block);
    EmitBlock(body_block);
  }

  llvm::BasicBlock *continue_block;
  if (node->GetInc()) {
    continue_block = CreateBasicBlock("for.inc");
  } else {
    continue_block = cond_block;
  }

  PushBlock(end_block, continue_block);
  EmitStmt(node->GetBlock());
  PopBlock();

  if (auto inc{node->GetInc()}) {
    EmitBlock(continue_block);
    inc->Accept(*this);
  }

  EmitBranch(cond_block);

  EmitBlock(end_block, true);
}

void CodeGen::Visit(const GotoStmt *node) {
  TryEmitLocation(node);
  EmitBranchThroughCleanup(GetBasicBlockForLabel(node->GetLabel()));
}

void CodeGen::Visit(const ContinueStmt *node) {
  TryEmitLocation(node);

  if (std::empty(break_continue_stack_)) {
    Error(node->GetLoc(), "continue stmt not in a loop or switch");
  } else {
    EmitBranchThroughCleanup(break_continue_stack_.top().continue_block);
  }
}

void CodeGen::Visit(const BreakStmt *node) {
  TryEmitLocation(node);

  if (std::empty(break_continue_stack_)) {
    Error(node->GetLoc(), "break stmt not in a loop or switch");
  } else {
    EmitBranchThroughCleanup(break_continue_stack_.top().break_block);
  }
}

void CodeGen::Visit(const ReturnStmt *node) {
  TryEmitLocation(node);

  auto expr{node->GetExpr()};

  if (!expr) {
    EmitBranchThroughCleanup(return_block_);
    return;
  }

  if (return_value_) {
    Load_Struct_Obj();
    node->GetExpr()->Accept(*this);
    Finish_Load();
    Builder.CreateStore(result_, return_value_);
  } else {
    Error(node->GetLoc(), "void function '{}' should not return a value",
          func_->getName().str());
  }

  EmitBranchThroughCleanup(return_block_);
}

}  // namespace kcc
