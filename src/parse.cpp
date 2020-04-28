//
// Created by kaiser on 2019/10/31.
//

#include "parse.h"

#include <algorithm>
#include <cassert>
#include <limits>

#include "calc.h"
#include "error.h"
#include "llvm_common.h"

namespace kcc {

Parser::Parser(std::vector<Token> tokens) : tokens_{std::move(tokens)} {
  Location loc;
  loc.SetFileName(Module->getSourceFileName());
  unit_ = MakeAstNode<TranslationUnit>(loc);

  AddBuiltin();
}

TranslationUnit* Parser::ParseTranslationUnit() {
  while (HasNext()) {
    unit_->AddExtDecl(ParseExternalDecl());
  }

  return unit_;
}

bool Parser::HasNext() { return !Peek().TagIs(Tag::kEof); }

const Token& Parser::Peek() { return tokens_[index_]; }

const Token& Parser::Next() { return tokens_[index_++]; }

void Parser::PutBack() {
  assert(index_ > 0);
  --index_;
}

bool Parser::Test(Tag tag) { return Peek().TagIs(tag); }

bool Parser::Try(Tag tag) {
  if (Test(tag)) {
    Next();
    return true;
  } else {
    return false;
  }
}

const Token& Parser::Expect(Tag tag) {
  if (!Test(tag)) {
    Error(tag, Peek());
  } else {
    return Next();
  }
}

void Parser::EnterBlock(Type* func_type) {
  scope_ = Scope::Get(scope_, kBlock);

  if (func_type) {
    for (const auto& param : func_type->FuncGetParams()) {
      scope_->InsertUsual(param);
    }
  }
}

void Parser::ExitBlock() { scope_ = scope_->GetParent(); }

void Parser::EnterFunc(IdentifierExpr* ident) {
  func_def_ = MakeAstNode<FuncDef>(ident->GetLoc(), ident);
}

void Parser::ExitFunc() { func_def_ = nullptr; }

void Parser::EnterProto() { scope_ = Scope::Get(scope_, kFuncProto); }

void Parser::ExitProto() { scope_ = scope_->GetParent(); }

bool Parser::IsTypeName(const Token& tok) {
  if (tok.IsTypeSpecQual()) {
    return true;
  } else if (tok.IsIdentifier()) {
    auto ident{scope_->FindUsual(tok)};
    if (ident && ident->IsTypeName()) {
      return true;
    }
  }

  return false;
}

bool Parser::IsDecl(const Token& tok) {
  if (tok.IsDeclSpec()) {
    return true;
  } else if (tok.IsIdentifier()) {
    auto ident{scope_->FindUsual(tok)};
    if (ident && ident->IsTypeName()) {
      return true;
    }
  }

  return false;
}

std::int64_t Parser::ParseInt64Constant() {
  auto expr{ParseExpr()};
  if (!expr->GetType()->IsIntegerTy()) {
    Error(expr, "expect integer");
  }

  auto val{*CalcConstantExpr{}.CalcInteger(expr)};

  return val;
}

LabelStmt* Parser::FindLabel(const std::string& name) const {
  if (auto iter{labels_.find(name)}; iter != std::end(labels_)) {
    return iter->second;
  } else {
    return nullptr;
  }
}

auto Parser::GetStructDesignator(Type* type, const std::string& name)
    -> decltype(std::begin(type->StructGetMembers())) {
  auto iter{std::begin(type->StructGetMembers())};

  for (; iter != std::end(type->StructGetMembers()); ++iter) {
    if ((*iter)->IsAnonymous()) {
      auto anonymous_type{(*iter)->GetType()};
      if (anonymous_type->StructGetMember(name)) {
        return iter;
      }
    } else if ((*iter)->GetName() == name) {
      return iter;
    }
  }

  assert(false);
  return iter;
}

Declaration* Parser::MakeDeclaration(const Token& token, QualType type,
                                     std::uint32_t storage_class_spec,
                                     std::uint32_t func_spec,
                                     std::int32_t align) {
  auto name{token.GetIdentifier()};

  if (storage_class_spec & kTypedef) {
    if (align > 0) {
      Error(token, "'_Alignas' attribute applies to typedef");
    }

    auto ident{scope_->FindUsualInCurrScope(name)};
    if (ident) {
      // 如果两次定义的类型兼容是可以的
      if (!type->Compatible(ident->GetType())) {
        Error(token, "typedef redefinition with different types '{}' vs '{}'",
              type->ToString(), ident->GetType()->ToString());
      } else {
        Warning(token, "Typedef redefinition");
        return nullptr;
      }
    } else {
      scope_->InsertUsual(name, MakeAstNode<IdentifierExpr>(
                                    token, name, type, Linkage::kNone, true));

      // 如果没有名字, 将 typedef 的名字给该 struct / union
      if (type->IsStructOrUnionTy() && !type->StructHasName()) {
        type->StructSetName(name);
      }

      return nullptr;
    }
  } else if (storage_class_spec & kRegister) {
    if (align > 0) {
      Error(token, "'_Alignas' attribute applies to register");
    }
  }

  if (type->IsVoidTy()) {
    Error(token, "variable or field '{}' declared void", name);
  } else if (type->IsFunctionTy() && !scope_->IsFileScope()) {
    Error(token, "function declaration is not allowed here");
  }

  Linkage linkage;
  if (scope_->IsFileScope()) {
    if (storage_class_spec & kStatic) {
      linkage = Linkage::kInternal;
    } else {
      linkage = Linkage::kExternal;
    }
  } else {
    linkage = Linkage::kNone;
  }

  auto ident{scope_->FindUsualInCurrScope(name)};
  //有链接对象(外部或内部)的声明可以重复
  if (ident) {
    if (!type->Compatible(ident->GetType())) {
      Error(token, "conflicting types '{}' vs '{}'", type->ToString(),
            ident->GetType()->ToString());
    }

    if (linkage == Linkage::kNone) {
      Error(token, "redefinition of '{}'", name);
    } else if (linkage == Linkage::kExternal) {
      // static int a = 1;
      // extern int a;
      // 这种情况是可以的
      if (ident->GetLinkage() == Linkage::kNone) {
        Error(token, "conflicting linkage '{}'", name);
      }
    } else {
      if (ident->GetLinkage() != Linkage::kInternal) {
        Error(token, "conflicting linkage '{}'", name);
      }
    }

    // extern int a;
    // int a = 1;
    if (auto obj{ident->ToObjectExpr()}) {
      if (!(storage_class_spec & kExtern)) {
        obj->SetStorageClassSpec(obj->GetStorageClassSpec() & ~kExtern);
      }

      if (!ident->GetType()->IsComplete() && type->IsComplete()) {
        ident->ToObjectExpr()->SetType(type.GetType());
      }

      auto decl{ident->ToObjectExpr()->GetDecl()};
      assert(decl != nullptr);
      return decl;
    }
  }

  // int a;
  // { extern int a;}
  if (storage_class_spec & kExtern) {
    ident = scope_->FindUsual(name);
    if (ident) {
      if (!type->Compatible(ident->GetType())) {
        Error(token, "conflicting types '{}' vs '{}'", type->ToString(),
              ident->GetType()->ToString());
      }

      if (ident->IsObject()) {
        if (!ident->GetType()->IsComplete() && type->IsComplete()) {
          ident->ToObjectExpr()->SetType(type.GetType());
        }

        auto decl{ident->ToObjectExpr()->GetDecl()};
        assert(decl != nullptr);
        return decl;
      }
    }
  }

  if (type->IsFunctionTy()) {
    if (align > 0) {
      Error(token, "'_Alignas' attribute applies to func");
    }

    type->FuncSetFuncSpec(func_spec);
    type->FuncSetName(name);

    ident = MakeAstNode<IdentifierExpr>(token, name, type, linkage, false);
    scope_->InsertUsual(name, ident);

    return MakeAstNode<Declaration>(token, ident);
  } else {
    auto obj{MakeAstNode<ObjectExpr>(token, name, type, storage_class_spec,
                                     linkage, false)};
    if (scope_->IsBlockScope() && storage_class_spec & kStatic) {
      obj->SetFuncName(func_def_->GetFuncType()->FuncGetName() +
                       std::to_string(reinterpret_cast<std::intptr_t>(scope_)));
    }

    if (align > 0) {
      if (align < type->GetWidth()) {
        Error(token,
              "requested alignment is less than minimum alignment of {} for "
              "type '{}'",
              type->GetWidth(), type.ToString());
      }
      obj->SetAlign(align);
    }

    scope_->InsertUsual(obj);
    auto decl{MakeAstNode<Declaration>(token, obj)};
    obj->SetDecl(decl);

    return decl;
  }
}

/*
 * ExtDecl
 */
ExtDecl* Parser::ParseExternalDecl() {
  auto ext_decl{ParseDecl(true)};

  // _Static_assert / e.g. int;
  if (ext_decl == nullptr) {
    return nullptr;
  }

  TryParseAsm();
  TryParseAttributeSpec();

  if (Test(Tag::kLeftBrace)) {
    auto stmt{ext_decl->GetStmts()};
    if (std::size(stmt) != 1) {
      Error(Peek(), "unexpect left braces");
    }

    return ParseFuncDef(dynamic_cast<Declaration*>(stmt.front()));
  } else {
    Expect(Tag::kSemicolon);
    return ext_decl;
  }
}

FuncDef* Parser::ParseFuncDef(const Declaration* decl) {
  auto ident{decl->GetIdent()};
  if (!ident->GetType()->IsFunctionTy()) {
    Error(decl->GetLoc(), "func def need func type");
  }

  EnterFunc(ident);
  func_def_->SetBody(ParseCompoundStmt(ident->GetType()));
  auto ret{func_def_};
  ExitFunc();

  // label 具有函数作用域
  for (auto&& item : gotos_) {
    auto label{FindLabel(item->GetName())};
    if (label) {
      item->SetLabel(label);
    } else {
      Error(item->GetLoc(), "unknown label: {}", item->GetName());
    }
  }
  gotos_.clear();
  labels_.clear();

  return ret;
}

/*
 * GNU 扩展
 */
// attribute-specifier:
//  __ATTRIBUTE__ '(' '(' attribute-list-opt ')' ')'
//
// attribute-list:
//  attribute-opt
//  attribute-list ',' attribute-opt
//
// attribute:
//  attribute-name
//  attribute-name '(' ')'
//  attribute-name '(' parameter-list ')'
//
// attribute-name:
//  identifier
//
// parameter-list:
//  identifier
//  identifier ',' expression-list
//  expression-list-opt
//
// expression-list:
//  expression
//  expression-list ',' expression
// 可以有多个
void Parser::TryParseAttributeSpec() {
  while (Try(Tag::kAttribute)) {
    Expect(Tag::kLeftParen);
    Expect(Tag::kLeftParen);

    ParseAttributeList();

    Expect(Tag::kRightParen);
    Expect(Tag::kRightParen);
  }
}

void Parser::ParseAttributeList() {
  while (!Test(Tag::kRightParen)) {
    ParseAttribute();

    if (!Test(Tag::kRightParen)) {
      Expect(Tag::kComma);
    }
  }
}

void Parser::ParseAttribute() {
  Expect(Tag::kIdentifier);

  if (Try(Tag::kLeftParen)) {
    ParseAttributeParamList();
    Expect(Tag::kRightParen);
  }
}

void Parser::ParseAttributeParamList() {
  if (Try(Tag::kIdentifier)) {
    if (Try(Tag::kComma)) {
      ParseAttributeExprList();
    }
  } else {
    ParseAttributeExprList();
  }
}

void Parser::ParseAttributeExprList() {
  while (!Test(Tag::kRightParen)) {
    ParseExpr();

    if (!Test(Tag::kRightParen)) {
      Expect(Tag::kComma);
    }
  }
}

void Parser::TryParseAsm() {
  if (Try(Tag::kAsm)) {
    Expect(Tag::kLeftParen);
    ParseStringLiteral();
    Expect(Tag::kRightParen);
  }
}

QualType Parser::ParseTypeof() {
  Expect(Tag::kLeftParen);

  QualType type;

  if (!IsTypeName(Peek())) {
    auto expr{ParseExpr()};
    type = expr->GetQualType();
  } else {
    type = ParseTypeName();
  }

  Expect(Tag::kRightParen);

  return type;
}

Expr* Parser::TryParseStmtExpr() {
  Try(Tag::kExtension);

  if (Try(Tag::kLeftParen)) {
    if (Test(Tag::kLeftBrace)) {
      return ParseStmtExpr();
    } else {
      PutBack();
    }
  }

  return nullptr;
}

Expr* Parser::ParseStmtExpr() {
  auto block{ParseCompoundStmt()};
  Expect(Tag::kRightParen);
  return MakeAstNode<StmtExpr>(block->GetLoc(), block);
}

Expr* Parser::ParseTypeid() {
  auto token{Expect(Tag::kLeftParen)};
  auto expr{ParseExpr()};
  Expect(Tag::kRightParen);

  auto str{expr->GetType()->ToString()};
  return MakeAstNode<StringLiteralExpr>(token, str);
}

/*
 * built in
 */
Expr* Parser::ParseOffsetof() {
  Expect(Tag::kLeftParen);

  auto token{Peek()};
  if (!IsTypeName(token)) {
    Error(token, "expect type name");
  }
  auto type{ParseTypeName()};

  if (!type->IsStructOrUnionTy()) {
    Error(token, "expect struct or union type");
  }

  Expect(Tag::kComma);

  std::uint64_t offset{};

  while (true) {
    token = Expect(Tag::kIdentifier);
    auto name{token.GetIdentifier()};
    auto obj{type->StructGetMember(name)};

    if (obj->GetBitFieldWidth()) {
      Error(token, "cannot compute offset of bit-field '{}'", obj->GetName());
    }

    offset += obj->GetOffset();

    if (!Try(Tag::kPeriod)) {
      Expect(Tag::kRightParen);
      break;
    }

    type = obj->GetType();
  }

  return MakeAstNode<ConstantExpr>(
      token, ArithmeticType::Get(kLong | kUnsigned), offset);
}

Expr* Parser::ParseHugeVal() {
  auto tok{Expect(Tag::kLeftParen)};
  Expect(Tag::kRightParen);

  return MakeAstNode<ConstantExpr>(
      tok, ArithmeticType::Get(kDouble),
      std::to_string(std::numeric_limits<double>::infinity()));
}

Expr* Parser::ParseInff() {
  auto tok{Expect(Tag::kLeftParen)};
  Expect(Tag::kRightParen);

  return MakeAstNode<ConstantExpr>(
      tok, ArithmeticType::Get(kFloat),
      std::to_string(std::numeric_limits<float>::infinity()));
}

void Parser::AddBuiltin() {
  auto loc{unit_->GetLoc()};

  auto va_list{StructType::Get(true, "__va_list_tag", scope_)};
  va_list->AddMember(
      MakeAstNode<ObjectExpr>(loc, "gp_offset", ArithmeticType::Get(kInt)));
  va_list->AddMember(
      MakeAstNode<ObjectExpr>(loc, "fp_offset", ArithmeticType::Get(kInt)));
  va_list->AddMember(MakeAstNode<ObjectExpr>(
      loc, "overflow_arg_area", PointerType::Get(VoidType::Get())));
  va_list->AddMember(MakeAstNode<ObjectExpr>(
      loc, "reg_save_area", PointerType::Get(VoidType::Get())));
  va_list->SetComplete(true);

  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(loc, "__builtin_va_list",
                                                  ArrayType::Get(va_list, 1),
                                                  Linkage::kNone, true));

  auto va_list_ptr{MakeAstNode<ObjectExpr>(loc, "", va_list->GetPointerTo())};
  auto integer{MakeAstNode<ObjectExpr>(loc, "", ArithmeticType::Get(kInt))};

  auto start{FunctionType::Get(VoidType::Get(), {va_list_ptr, integer})};
  start->SetName("__builtin_va_start");
  auto end{FunctionType::Get(VoidType::Get(), {va_list_ptr})};
  end->SetName("__builtin_va_end");
  auto arg{FunctionType::Get(VoidType::Get(), {va_list_ptr})};
  arg->SetName("__builtin_va_arg_sub");
  auto copy{FunctionType::Get(VoidType::Get(), {va_list_ptr, va_list_ptr})};
  copy->SetName("__builtin_va_copy");

  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(
      loc, "__builtin_va_start", start, Linkage::kExternal, false));
  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(loc, "__builtin_va_end", end,
                                                  Linkage::kExternal, false));
  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(
      loc, "__builtin_va_arg_sub", arg, Linkage::kExternal, false));
  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(
      loc, "__builtin_va_copy", copy, Linkage::kExternal, false));

  auto sync_synchronize{FunctionType::Get(VoidType::Get(), {})};
  sync_synchronize->FuncSetName("__sync_synchronize");
  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(
      loc, "__sync_synchronize", sync_synchronize, Linkage::kExternal, false));

  auto ulong{
      MakeAstNode<ObjectExpr>(loc, "", ArithmeticType::Get(kLong | kUnsigned))};
  auto alloca{
      FunctionType::Get(ArithmeticType::Get(kChar)->GetPointerTo(), {ulong})};
  alloca->FuncSetName("__builtin_alloca");
  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(
      loc, "__builtin_alloca", alloca, Linkage::kExternal, false));

  auto popcount{FunctionType::Get(ArithmeticType::Get(kInt), {integer})};
  popcount->FuncSetName("__builtin_popcount");
  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(
      loc, "__builtin_popcount", popcount, Linkage::kExternal, false));

  auto clz{FunctionType::Get(ArithmeticType::Get(kInt), {integer})};
  clz->FuncSetName("__builtin_clz");
  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(loc, "__builtin_clz", clz,
                                                  Linkage::kExternal, false));

  auto ctz{FunctionType::Get(ArithmeticType::Get(kInt), {integer})};
  ctz->FuncSetName("__builtin_ctz");
  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(loc, "__builtin_ctz", ctz,
                                                  Linkage::kExternal, false));

  auto long_integer{
      MakeAstNode<ObjectExpr>(loc, "", ArithmeticType::Get(kLong))};
  auto expect{FunctionType::Get(ArithmeticType::Get(kLong),
                                {long_integer, long_integer})};
  expect->FuncSetName("__builtin_expect");
  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(
      loc, "__builtin_expect", expect, Linkage::kExternal, false));

  auto float_param{
      MakeAstNode<ObjectExpr>(loc, "", ArithmeticType::Get(kFloat))};
  auto isinf_sign{FunctionType::Get(ArithmeticType::Get(kInt), {float_param})};
  isinf_sign->FuncSetName("__builtin_isinf_sign");
  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(
      loc, "__builtin_isinf_sign", isinf_sign, Linkage::kExternal, false));

  auto isfinite{FunctionType::Get(ArithmeticType::Get(kInt), {float_param})};
  isfinite->FuncSetName("__builtin_isfinite");
  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(
      loc, "__builtin_isfinite", isfinite, Linkage::kExternal, false));

  auto bswap32{FunctionType::Get(ArithmeticType::Get(kInt), {integer})};
  bswap32->FuncSetName("__builtin_bswap32");
  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(
      loc, "__builtin_bswap32", bswap32, Linkage::kExternal, false));

  auto bswap64{FunctionType::Get(ArithmeticType::Get(kLong), {long_integer})};
  bswap64->FuncSetName("__builtin_bswap64");
  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(
      loc, "__builtin_bswap64", bswap64, Linkage::kExternal, false));

  auto short_integer{
      MakeAstNode<ObjectExpr>(loc, "", ArithmeticType::Get(kShort))};
  auto bswap16{FunctionType::Get(ArithmeticType::Get(kShort), {short_integer})};
  bswap16->FuncSetName("__builtin_bswap16");
  scope_->InsertUsual(MakeAstNode<IdentifierExpr>(
      loc, "__builtin_bswap16", bswap16, Linkage::kExternal, false));
}

}  // namespace kcc
