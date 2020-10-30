//
// Created by kaiser on 2019/12/13.
//

#include "parse.h"

#include "error.h"

namespace kcc {

/*
 * Stmt
 */
Stmt *Parser::ParseStmt() {
  TryParseAttributeSpec();

  switch (Peek().GetTag()) {
    case Tag::kIdentifier: {
      Next();
      if (Peek().TagIs(Tag::kColon)) {
        PutBack();
        return ParseLabelStmt();
      } else {
        PutBack();
        return ParseExprStmt();
      }
    }
    case Tag::kCase:
      return ParseCaseStmt();
    case Tag::kDefault:
      return ParseDefaultStmt();
    case Tag::kLeftBrace:
      return ParseCompoundStmt();
    case Tag::kIf:
      return ParseIfStmt();
    case Tag::kSwitch:
      return ParseSwitchStmt();
    case Tag::kWhile:
      return ParseWhileStmt();
    case Tag::kDo:
      return ParseDoWhileStmt();
    case Tag::kFor:
      return ParseForStmt();
    case Tag::kGoto:
      return ParseGotoStmt();
    case Tag::kContinue:
      return ParseContinueStmt();
    case Tag::kBreak:
      return ParseBreakStmt();
    case Tag::kReturn:
      return ParseReturnStmt();
    default:
      return ParseExprStmt();
  }
}

Stmt *Parser::ParseLabelStmt() {
  auto token{Expect(Tag::kIdentifier)};
  Expect(Tag::kColon);

  TryParseAttributeSpec();

  auto name{token.GetIdentifier()};
  if (FindLabel(name)) {
    Error(token, "redefine of label: '{}'", token.GetIdentifier());
  }

  auto label{MakeAstNode<LabelStmt>(token, name, ParseStmt())};
  labels_[name] = label;

  return label;
}

Stmt *Parser::ParseCaseStmt() {
  auto token{Expect(Tag::kCase)};

  auto lhs{ParseInt64Constant()};

  if (Try(Tag::kEllipsis)) {
    auto rhs{ParseInt64Constant()};
    Expect(Tag::kColon);
    return MakeAstNode<CaseStmt>(token, lhs, rhs, ParseStmt());
  } else {
    Expect(Tag::kColon);
    return MakeAstNode<CaseStmt>(token, lhs, ParseStmt());
  }
}

Stmt *Parser::ParseDefaultStmt() {
  auto token{Expect(Tag::kDefault)};
  Expect(Tag::kColon);

  return MakeAstNode<DefaultStmt>(token, ParseStmt());
}

CompoundStmt *Parser::ParseCompoundStmt(Type *func_type) {
  auto token{Expect(Tag::kLeftBrace)};

  EnterBlock(func_type);

  auto stmts{MakeAstNode<CompoundStmt>(token)};
  compound_stmt_.push(stmts);

  while (!Try(Tag::kRightBrace)) {
    if (IsDecl(Peek())) {
      stmts->AddStmt(ParseDecl());
    } else {
      stmts->AddStmt(ParseStmt());
    }
  }

  ExitBlock();
  compound_stmt_.pop();

  return stmts;
}

Stmt *Parser::ParseExprStmt() {
  auto token{Peek()};
  if (Try(Tag::kSemicolon)) {
    return MakeAstNode<ExprStmt>(token);
  } else {
    auto ret{MakeAstNode<ExprStmt>(token, ParseExpr())};
    Expect(Tag::kSemicolon);
    return ret;
  }
}

Stmt *Parser::ParseIfStmt() {
  auto token{Expect(Tag::kIf)};

  Expect(Tag::kLeftParen);
  auto cond{ParseExpr()};
  Expect(Tag::kRightParen);

  auto then_block{ParseStmt()};
  if (Try(Tag::kElse)) {
    return MakeAstNode<IfStmt>(token, cond, then_block, ParseStmt());
  } else {
    return MakeAstNode<IfStmt>(token, cond, then_block);
  }
}

Stmt *Parser::ParseSwitchStmt() {
  auto token{Expect(Tag::kSwitch)};

  Expect(Tag::kLeftParen);
  auto cond{ParseExpr()};
  Expect(Tag::kRightParen);

  return MakeAstNode<SwitchStmt>(token, cond, ParseStmt());
}

Stmt *Parser::ParseWhileStmt() {
  auto token{Expect(Tag::kWhile)};

  Expect(Tag::kLeftParen);
  auto cond{ParseExpr()};
  Expect(Tag::kRightParen);

  return MakeAstNode<WhileStmt>(token, cond, ParseStmt());
}

Stmt *Parser::ParseDoWhileStmt() {
  auto token{Expect(Tag::kDo)};

  auto stmt{ParseStmt()};

  Expect(Tag::kWhile);
  Expect(Tag::kLeftParen);
  auto cond{ParseExpr()};
  Expect(Tag::kRightParen);
  Expect(Tag::kSemicolon);

  return MakeAstNode<DoWhileStmt>(token, cond, stmt);
}

Stmt *Parser::ParseForStmt() {
  auto token{Expect(Tag::kFor)};
  Expect(Tag::kLeftParen);

  Expr *init{}, *cond{}, *inc{};
  Stmt *block{};
  Stmt *decl{};

  EnterBlock();
  if (IsDecl(Peek())) {
    decl = ParseDecl(false);
  } else if (!Try(Tag::kSemicolon)) {
    init = ParseExpr();
    Expect(Tag::kSemicolon);
  }

  if (!Try(Tag::kSemicolon)) {
    cond = ParseExpr();
    Expect(Tag::kSemicolon);
  }

  if (!Try(Tag::kRightParen)) {
    inc = ParseExpr();
    Expect(Tag::kRightParen);
  }

  block = ParseStmt();
  ExitBlock();

  return MakeAstNode<ForStmt>(token, init, cond, inc, block, decl);
}

Stmt *Parser::ParseGotoStmt() {
  Expect(Tag::kGoto);
  auto tok{Expect(Tag::kIdentifier)};
  Expect(Tag::kSemicolon);

  auto ret{MakeAstNode<GotoStmt>(tok, tok.GetIdentifier())};
  gotos_.push_back(ret);

  return ret;
}

Stmt *Parser::ParseContinueStmt() {
  auto token{Expect(Tag::kContinue)};
  Expect(Tag::kSemicolon);

  return MakeAstNode<ContinueStmt>(token);
}

Stmt *Parser::ParseBreakStmt() {
  auto token{Expect(Tag::kBreak)};
  Expect(Tag::kSemicolon);

  return MakeAstNode<BreakStmt>(token);
}

Stmt *Parser::ParseReturnStmt() {
  auto token{Expect(Tag::kReturn)};

  if (Try(Tag::kSemicolon)) {
    return MakeAstNode<ReturnStmt>(token);
  } else {
    auto expr{ParseExpr()};
    expr = Expr::MayCastTo(expr, func_def_->GetFuncType()->FuncGetReturnType());

    Expect(Tag::kSemicolon);

    return MakeAstNode<ReturnStmt>(token, expr);
  }
}

}  // namespace kcc
