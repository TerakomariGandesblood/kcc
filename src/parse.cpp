//
// Created by kaiser on 2019/10/31.
//

#include "parse.h"

#include "calc.h"
#include "error.h"
#include "lex.h"

namespace kcc {

Parser::Parser(std::vector<Token> tokens) : tokens_{std::move(tokens)} {}

std::shared_ptr<TranslationUnit> Parser::ParseTranslationUnit() {
  auto unit{MakeAstNode<TranslationUnit>()};

  while (HasNext()) {
    unit->AddStmt(ParseExternalDecl());
  }

  return unit;
}

bool Parser::HasNext() { return !Peek().TagIs(Tag::kEof); }

Token Parser::Peek() { return tokens_[index_]; }

Token Parser::Next() { return tokens_[index_++]; }

void Parser::PutBack() { --index_; }

bool Parser::Test(Tag tag) { return Peek().TagIs(tag); }

bool Parser::Try(Tag tag) {
  if (Test(tag)) {
    Next();
    return true;
  } else {
    return false;
  }
}

void Parser::ParseStaticAssertDecl() {
  Expect(Tag::kLeftParen);
  auto expr{ParseConstantExpr()};
  Expect(Tag::kComma);

  auto msg{ParseStringLiteral(false)};
  Expect(Tag::kRightParen);
  Expect(Tag::kSemicolon);

  if (!Calculation<std::int32_t>{}.Calc(expr)) {
    Error(expr->GetToken(), "static_assert failed \"{}\"", msg);
  }
}

Token Parser::PeekPrev() { return tokens_[index_ - 1]; }

Token Parser::Expect(Tag tag) {
  if (!Test(tag)) {
    Error(tag, Peek());
  } else {
    return Next();
  }
}

std::shared_ptr<Expr> Parser::ParseExpr() { return ParseCommaExpr(); }

std::shared_ptr<Expr> Parser::ParseCommaExpr() {
  auto lhs{ParseAssignExpr()};

  MarkLoc(Peek().GetLoc());
  while (Try(Tag::kComma)) {
    auto rhs{ParseAssignExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kComma, std::move(lhs), rhs);

    MarkLoc(Peek().GetLoc());
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseAssignExpr() {
  auto lhs{ParseConditionExpr()};
  std::shared_ptr<Expr> rhs;

  auto tok{Next()};
  MarkLoc(tok.GetLoc());

  switch (tok.GetTag()) {
    case Tag::kEqual:
      rhs = ParseAssignExpr();
      break;
    case Tag::kStarEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kStar, lhs, rhs);
      break;
    case Tag::kSlashEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kSlash, lhs, rhs);
      break;
    case Tag::kPercentEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kPercent, lhs, rhs);
      break;
    case Tag::kPlusEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kPlus, lhs, rhs);
      break;
    case Tag::kMinusEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kMinus, lhs, rhs);
      break;
    case Tag::kLessLessEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kLessLess, lhs, rhs);
      break;
    case Tag::kGreaterGreaterEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kGreaterGreater, lhs, rhs);
      break;
    case Tag::kAmpEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kAmp, lhs, rhs);
      break;
    case Tag::kCaretEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kCaret, lhs, rhs);
      break;
    case Tag::kPipeEqual:
      rhs = ParseAssignExpr();
      rhs = MakeAstNode<BinaryOpExpr>(Tag::kPipe, lhs, rhs);
      break;
    default: {
      PutBack();
      return lhs;
    }
  }

  return MakeAstNode<BinaryOpExpr>(Tag::kEqual, lhs, rhs);
}

std::shared_ptr<Expr> Parser::ParseConditionExpr() {
  auto cond{ParseLogicalOrExpr()};
  MarkLoc(Peek().GetLoc());

  // 条件运算符 ? 与 : 之间的表达式分析为如同加括号，忽略其相对于 ?: 的优先级
  if (Try(Tag::kQuestion)) {
    // GNU 扩展
    // a ?: b 相当于 a ? a: c
    auto true_expr{Test(Tag::kColon) ? cond : ParseExpr()};
    Expect(Tag::kColon);
    auto false_expr{ParseExpr()};

    return MakeAstNode<ConditionOpExpr>(cond, true_expr, false_expr);
  }

  return cond;
}

std::shared_ptr<Expr> Parser::ParseLogicalOrExpr() {
  auto lhs{ParseLogicalAndExpr()};
  MarkLoc(Peek().GetLoc());

  while (Try(Tag::kPipePipe)) {
    auto rhs{ParseLogicalAndExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kPipePipe, lhs, rhs);

    MarkLoc(Peek().GetLoc());
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseLogicalAndExpr() {
  auto lhs{ParseBitwiseOrExpr()};
  MarkLoc(Peek().GetLoc());

  while (Try(Tag::kAmpAmp)) {
    auto rhs{ParseBitwiseOrExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kAmpAmp, lhs, rhs);

    MarkLoc(Peek().GetLoc());
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseBitwiseOrExpr() {
  auto lhs{ParseBitwiseXorExpr()};
  MarkLoc(Peek().GetLoc());

  while (Try(Tag::kPipe)) {
    auto rhs{ParseBitwiseXorExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kPipe, lhs, rhs);

    MarkLoc(Peek().GetLoc());
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseBitwiseXorExpr() {
  auto lhs{ParseBitwiseAndExpr()};
  MarkLoc(Peek().GetLoc());

  while (Try(Tag::kCaret)) {
    auto rhs{ParseBitwiseAndExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kCaret, lhs, rhs);

    MarkLoc(Peek().GetLoc());
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseBitwiseAndExpr() {
  auto lhs{ParseEqualityExpr()};
  MarkLoc(Peek().GetLoc());

  while (Try(Tag::kAmp)) {
    auto rhs{ParseEqualityExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(Tag::kAmp, lhs, rhs);

    MarkLoc(Peek().GetLoc());
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseEqualityExpr() {
  auto lhs{ParseRelationExpr()};
  MarkLoc(Peek().GetLoc());

  while (true) {
    if (Try(Tag::kEqual)) {
      auto rhs{ParseRelationExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kEqual, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kExclaimEqual)) {
      auto rhs{ParseRelationExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kExclaimEqual, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else {
      break;
    }
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseRelationExpr() {
  auto lhs{ParseShiftExpr()};
  MarkLoc(Peek().GetLoc());

  while (true) {
    if (Try(Tag::kLess)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kLess, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kLessEqual)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kLessEqual, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kGreater)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kGreater, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kGreaterEqual)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kGreaterEqual, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else {
      break;
    }
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseShiftExpr() {
  auto lhs{ParseAdditiveExpr()};
  MarkLoc(Peek().GetLoc());

  while (true) {
    if (Try(Tag::kLessLess)) {
      auto rhs{ParseAdditiveExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kLessLess, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kGreaterGreater)) {
      auto rhs{ParseAdditiveExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kGreaterGreater, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else {
      break;
    }
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseAdditiveExpr() {
  auto lhs{ParseMultiplicativeExpr()};
  MarkLoc(Peek().GetLoc());

  while (true) {
    if (Try(Tag::kPlus)) {
      auto rhs{ParseMultiplicativeExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kPlus, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kMinus)) {
      auto rhs{ParseMultiplicativeExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kMinus, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else {
      break;
    }
  }

  return lhs;
}

std::shared_ptr<Expr> Parser::ParseMultiplicativeExpr() {
  auto lhs{ParseCastExpr()};
  MarkLoc(Peek().GetLoc());

  while (true) {
    if (Try(Tag::kStar)) {
      auto rhs{ParseCastExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kStar, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kSlash)) {
      auto rhs{ParseCastExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kSlash, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else if (Try(Tag::kPercent)) {
      auto rhs{ParseCastExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(Tag::kPercent, lhs, rhs);
      MarkLoc(Peek().GetLoc());
    } else {
      break;
    }
  }

  return lhs;
}

std::shared_ptr<CompoundStmt> Parser::ParseDecl() {
  if (Try(Tag::kStaticAssert)) {
    ParseStaticAssertDecl();
    return nullptr;
  } else {
    auto base_type{ParseDeclSpec()};

    if (Test(Tag::kSemicolon)) {
      Warning(Next(), "declaration does not declare anything");
      return nullptr;
    } else {
      auto ret{ParseInitDeclaratorList(base_type)};
      Expect(Tag::kSemicolon);
      return ret;
    }
  }
}

std::shared_ptr<CompoundStmt> Parser::ParseInitDeclaratorList(
    std::shared_ptr<Type>& base_type) {
  auto init_decls{std::make_shared<CompoundStmt>()};

  do {
    init_decls->AddStmt(
        ParseInitDeclarator(base_type, storage_class_spec, func_spec, align));
  } while (Try(Tag::kComma));

  return init_decls;
}

std::shared_ptr<Declaration> Parser::ParseInitDeclarator(
    std::shared_ptr<Type>& base_type) {
  std::string name;
  ParseDeclarator(name, base_type);

  auto decl{
      MakeDeclarator(name, base_type, storage_class_spec, func_spec, align)};

  if (Try(Tag::kEqual)) {
    decl->AddInits(ParseInitializer());
  }

  return decl;
}

void Parser::ParseDeclarator(std::string& name,
                             std::shared_ptr<Type>& base_type) {
  while (Try(Tag::kStar)) {
    base_type = base_type->GetPointerTo();
    // TODO type-qualifier
  }

  ParseDirectDeclarator(name, base_type);
}

void Parser::ParseDirectDeclarator(std::string& name,
                                   std::shared_ptr<Type>& base_type) {
  if (Test(Tag::kIdentifier)) {
    name = Next().GetStr();
    ParseDirectDeclaratorTail(base_type);
  } else if (Try(Tag::kLeftParen)) {
    auto begin{index_};
    auto temp{std::make_shared<Type>()};
    // 此时的 base_type 不一定是正确的, 先跳过括号中的内容
    ParseDeclarator(name, temp);
    Expect(Tag::kRightParen);

    ParseDirectDeclaratorTail(base_type);
    auto end{index_};

    index_ = begin;
    ParseDeclarator(name, base_type);
    Expect(Tag::kRightParen);
    index_ = end;
  } else {
    ParseDirectDeclaratorTail(base_type);
  }
}

// 不支持在 struct / union 中使用 _Alignas
// 在 struct / union 中和函数参数声明中只能使用 type specifier 和 type qualifier
std::shared_ptr<Type> Parser::ParseDeclSpec(bool in_struct_or_func) {
#define CheckAndSetStorageClassSpec(spec)                       \
  if (storage_class_spec != 0) {                                \
    Error(tok, "duplicated storage class specifier");           \
  } else if (in_struct_or_func) {                               \
    Error(tok, "storage class specifier are not allowed here"); \
  }                                                             \
  storage_class_spec |= spec;

#define CheckAndSetFuncSpec(spec)                                       \
  if (func_spec & spec) {                                               \
    Warning(tok, "duplicate function specifier declaration specifier"); \
  } else if (in_struct_or_func) {                                       \
    Error(tok, "function specifiers are not allowed here");             \
  }                                                                     \
  func_spec |= spec;

#define ERROR Error(tok, "two or more data types in declaration specifiers");

  std::uint32_t type_spec{}, type_qualifiers{}, storage_class_spec{},
      func_spec{};
  std::int32_t align{};

  Token tok;
  std::shared_ptr<Type> type;

  while (true) {
    tok = Next();

    switch (tok.GetTag()) {
      // Storage Class Specifier, 至多有一个
      case Tag::kTypedef:
        CheckAndSetStorageClassSpec(kTypedef) break;
      case Tag::kExtern:
        CheckAndSetStorageClassSpec(kExtern) break;
      case Tag::kStatic:
        CheckAndSetStorageClassSpec(kStatic) break;
      case Tag::kAuto:
        CheckAndSetStorageClassSpec(kAuto) break;
      case Tag::kRegister:
        CheckAndSetStorageClassSpec(kRegister) break;
      case Tag::kThreadLocal:
        Error(tok, "Does not support _Thread_local");

        // Type specifier
      case Tag::kVoid:
        if (type_spec) {
          ERROR
        }
        type_spec |= kVoid;
        break;
      case Tag::kChar:
        if (type_spec & ~kCompChar) {
          ERROR
        }
        type_spec |= kChar;
        break;
      case Tag::kShort:
        if (type_spec & ~kCompShort) {
          ERROR
        }
        type_spec |= kShort;
        break;
      case Tag::kInt:
        if (type_spec & ~kCompInt) {
          ERROR
        }
        type_spec |= kInt;
        break;
      case Tag::kLong:
        if (type_spec & ~kCompLong) {
          ERROR
        }

        if (type_spec & kLong) {
          type_spec &= ~kLong;
          type_spec |= kLongLong;
        } else {
          type_spec |= kLong;
        }
        break;
      case Tag::kFloat:
        if (type_spec) {
          ERROR
        }
        type_spec |= kFloat;
        break;
      case Tag::kDouble:
        if (type_spec & ~kCompDouble) {
          ERROR
        }
        type_spec |= kDouble;
        break;
      case Tag::kSigned:
        if (type_spec & ~kCompSigned) {
          ERROR
        }
        type_spec |= kSigned;
        break;
      case Tag::kUnsigned:
        if (type_spec & ~kCompUnsigned) {
          ERROR
        }
        type_spec |= kUnsigned;
        break;
      case Tag::kBool:
        if (type_spec) {
          ERROR
        }
        type_spec |= kBool;
        break;
      case Tag::kStruct:
      case Tag::kUnion:
        if (type_spec) {
          ERROR
        }
        type = ParseStructUnionSpec(tok.GetTag() == Tag::kStruct);
        type_spec |= kStructUnionSpec;
        break;
      case Tag::kEnum:
        if (type_spec) {
          ERROR
        }
        type = ParseEnumSpec();
        type_spec |= kEnumSpec;
        break;
      case Tag::kComplex:
        Error(tok, "Does not support _Complex");
      case Tag::kAtomic:
        Error(tok, "Does not support _Atomic");

        // Type qualifier
      case Tag::kConst:
        type_qualifiers |= kConst;
        break;
      case Tag::kRestrict:
        type_qualifiers |= kRestrict;
        break;
      case Tag::kVolatile:
        Error(tok, "Does not support volatile");

        // Function specifier
      case Tag::kInline:
        CheckAndSetFuncSpec(kInline) break;
      case Tag::kNoreturn:
        CheckAndSetFuncSpec(kNoreturn) break;

      case Tag::kAlignas:
        if (in_struct_or_func) {
          Error(tok, "_Alignas are not allowed here");
        }
        align = std::max(ParseAlignas(), align);
        break;

      default: {
        if (type_spec == 0 && IsTypeName(tok)) {
          auto ident{curr_scope_->FindNormal(tok.GetStr())};
          type = ident->GetType();
          // TODO ???
          type_spec |= kTypedefName;
        } else {
          goto finish;
        }
      }
    }
  }

finish:
  PutBack();

  if (!type_spec) {
    Error(tok, "type specifier missing");
  }

  type = Type::Get(type_spec);
  type->SetAlign(align);
  type->SetTypeQualifiers(type_qualifiers);
  type->SetFuncSpec(func_spec);
  type->SetStorageClassSpec(storage_class_spec);

  // TODO type attributes

  return type;

#undef CheckAndSetStorageClassSpec
#undef CheckAndSetFuncSpec
#undef ERROR
}

std::shared_ptr<Type> Parser::ParseStructUnionSpec(bool is_struct) {
  // TODO type attribute
  auto tok{Peek()};
  std::string name;

  if (Try(Tag::kIdentifier)) {
    name = tok.GetStr();
    if (Try(Tag::kLeftBrace)) {
      auto tag{curr_scope_->FindTagInCurrScope(name)};
      // 无前向声明
      if (!tag) {
        auto type{StructType::Get(is_struct, true, curr_scope_)};
        auto ident{std::make_shared<Identifier>(tok, type, kNone, true)};
        curr_scope_->InsertTag(name, ident);
        ParseStructDeclList(type);
        Expect(Tag::kRightBrace);
        return type;
      } else {
        if (tag->GetType()->IsComplete()) {
          Error(tok, "redefinition:{}", name);
        } else {
          ParseStructDeclList(
              std::dynamic_pointer_cast<StructType>(tag->GetType()));
          Expect(Tag::kRightBrace);
          return tag->GetType();
        }
      }
    } else {
      // 可能是前向声明或普通的声明
      auto tag{curr_scope_->FindTag(name)};

      if (tag) {
        return tag->GetType();
      }

      auto type{StructType::Get(is_struct, true, curr_scope_)};
      auto ident{std::make_shared<Identifier>(tok, type, kNone, true)};
      curr_scope_->InsertTag(name, ident);
      return type;
    }
  } else {
    // 无标识符只能是定义
    Expect(Tag::kLeftBrace);

    auto type{StructType::Get(is_struct, false, curr_scope_)};
    ParseStructDeclList(type);

    Expect(Tag::kRightBrace);
    return type;
  }
}

void Parser::ParseStructDeclList(std::shared_ptr<StructType> type) {
  auto scope_backup{curr_scope_};
  curr_scope_ = type->GetScope();

  while (!Test(Tag::kRightBrace)) {
    if (!HasNext()) {
      Error(PeekPrev(), "premature end of input");
    }

    if (Try(Tag::kStaticAssert)) {
      ParseStaticAssertDecl();
    } else {
      auto base_type{ParseDeclSpec(true)};

      do {
        std::string name;
        ParseDeclarator(name, base_type);

        // TODO 位域

        // 可能是匿名 struct / union
        if (std::empty(name)) {
          if (base_type->IsStructTy() && !base_type->HasStructName()) {
            auto anony{std::make_shared<Object>(Peek(), base_type)};
            type->MergeAnony(anony);
            continue;
          } else {
            Error(Peek(), "declaration does not declare anything");
          }
        } else {
          if (type->GetMember(name)) {
            Error(Peek(), "duplicate member:{}", name);
          } else if (!base_type->IsComplete()) {
            // 可能是柔性数组
            if (type->IsStruct() && std::size(type->GetMembers()) > 0 &&
                base_type->IsArrayTy()) {
              auto member{std::make_shared<Object>(Peek(), base_type)};
              type->AddMember(member);
              // 必须是最后一个成员
              Expect(Tag::kSemicolon);
              Expect(Tag::kSemicolon);
              goto finalize;
            } else {
              Error(Peek(), "error");
            }
          } else if (base_type->IsFunctionTy()) {
            Error(Peek(), "error");
          } else {
            auto member{std::make_shared<Object>(Peek(), base_type)};
            type->AddMember(member);
          }
        }
      } while (Try(Tag::kComma));
      Expect(Tag::kSemicolon);
    }
  }

finalize:
  // TODO type attributes

  type->SetComplete(true);

  // struct / union 中的 tag 的作用域与该 struct / union 所在的作用域相同
  for (const auto& [name, tag] : curr_scope_->AllTagInCurrScope()) {
    if (scope_backup->FindTagInCurrScope(name)) {
      Error(tag->GetToken(), "redefinition of tag {}", tag->GetName());
    } else {
      scope_backup->InsertTag(name, tag);
    }
  }

  curr_scope_ = scope_backup;
}

std::shared_ptr<Type> Parser::ParseEnumSpec() {
  // TODO type attributes

  std::string name;
  auto tok{Peek()};

  if (Try(Tag::kIdentifier)) {
    name = tok.GetStr();

    // 定义
    if (Try(Tag::kLeftBrace)) {
      auto tag{curr_scope_->FindTagInCurrScope(name)};

      if (!tag) {
        auto type{IntegerType::Get(32)};
        auto ident{std::make_shared<Identifier>(tok, type, kNone, true)};
        curr_scope_->InsertTag(name, ident);
        ParseEnumerator(type);
        Expect(Tag::kRightBrace);
        return type;
      } else {
        Error(tok, "error");
      }
    } else {
      // 只能是普通声明，不允许前向声明
      auto tag{curr_scope_->FindTag(name)};
      if (tag) {
        return tag->GetType();
      } else {
        Error(tok, "error");
      }
    }
  } else {
    Expect(Tag::kLeftBrace);
    auto type{IntegerType::Get(32)};
    ParseEnumerator(type);
    Expect(Tag::kRightBrace);
    return type;
  }
}

void Parser::ParseEnumerator(std::shared_ptr<Type> type) {
  std::int32_t val{};

  do {
    // TODO type attributes
    auto tok{Expect(Tag::kIdentifier)};
    auto name{tok.GetStr()};
    auto ident{curr_scope_->FindNormalInCurrScope(name)};

    if (ident) {
      Error(tok, "error");
    }

    if (Try(Tag::kEqual)) {
      auto expr{ParseConditionExpr()};
      val = Calculation<std::int32_t>{}.Calc(expr);
    }

    auto enumer{std::make_shared<Enumerator>(tok, val)};
    ++val;
    curr_scope_->InsertNormal(name, enumer);

    Try(Tag::kComma);
  } while (Test(Tag::kRightBrace));

  type->SetComplete(true);
}

std::int32_t Parser::ParseAlignas() {
  Expect(Tag::kLeftParen);

  std::int32_t align;
  auto tok{Peek()};

  if (IsTypeName(tok)) {
    auto type{ParseTypeName()};
    align = type->Align();
  } else {
    auto expr{ParseConditionExpr()};
    align = Calculation<std::int32_t>{}.Calc(expr);
  }

  Expect(Tag::kRightParen);

  if (align < 0 || ((align - 1) & align)) {
    Error(tok, "error");
  }

  return align;
}

void Parser::ParseDirectDeclaratorTail(std::shared_ptr<Type>& base_type) {
  if (Try(Tag::kLeftSquare)) {
    if (base_type->IsFunctionTy()) {
      Error("error");
    }

    auto len{ParseArrayLength()};
    Expect(Tag::kRightSquare);

    base_type = ArrayType::Get(base_type, len);
  } else if (Try(Tag::kLeftParen)) {
    if (base_type->IsFunctionTy()) {
      Error("error");
    } else if (base_type->IsArrayTy()) {
      Error("error");
    }

    EnterProto();
    auto [params, var]{ParseParamList()};
    ExitProto();
    Expect(Tag::kRightParen);

    base_type = FunctionType::Get(base_type, params, var);
  }
}

std::shared_ptr<Declaration> Parser::MakeDeclarator(
    const std::string& name, const std::shared_ptr<Type>& base_type) {
  return std::shared_ptr<Declaration>();
}

std::size_t Parser::ParseArrayLength() {
  // 不支持 type qualifier

  auto expr{ParseConditionExpr()};

  if (!expr->GetType()->IsIntegerTy()) {
    Error("error");
  }

  auto len{Calculation<std::int32_t>{}.Calc(expr)};

  if (len <= 0) {
    Error("error");
  }

  return len;
}

void Parser::EnterProto() {
  curr_scope_ = std::make_shared<Scope>(curr_scope_, kFuncProto);
}

void Parser::ExitProto() { curr_scope_ = curr_scope_->GetParent(); }

std::pair<std::vector<std::shared_ptr<Object>>, bool> Parser::ParseParamList() {
  if (Test(Tag::kRightParen)) {
    return {{}, false};
  }

  auto param{ParseParamDecl()};
  if (param->GetType()->IsVoidTy()) {
    return {{}, false};
  }

  std::vector<std::shared_ptr<Object>> params;
  params.push_back(param);

  while (Try(Tag::kComma)) {
    if (Try(Tag::kEllipsis)) {
      return {params, true};
    }

    param = ParseParamDecl();
    if (param->GetType()->IsVoidTy()) {
      Error("error");
    }
    params.push_back(param);
  }

  return {params, false};
}

std::shared_ptr<Object> Parser::ParseParamDecl() {}

bool Parser::IsTypeName(const Token& token) {
  if (token.IsTypeSpec()) {
    return true;
  }

  if (token.TagIs(Tag::kIdentifier)) {
    auto ident{curr_scope_->FindNormal(token.GetStr())};
    if (ident && ident->IsTypeName()) {
      return true;
    }
  }

  return false;
}

std::shared_ptr<Type> Parser::ParseTypeName() {
  auto type{ParseDeclSpec(true)};
}

std::shared_ptr<Expr> Parser::ParseConstantExpr() {
  return ParseConditionExpr();
}

std::shared_ptr<Constant> Parser::ParseStringLiteral(bool handle_escape) {
  auto tok{Expect(Tag::kStringLiteral)};

  auto str{Scanner{tok.GetStr()}.ScanStringLiteral(handle_escape)};
  while (Test(Tag::kStringLiteral)) {
    tok = Next();
    str += Scanner{tok.GetStr()}.ScanStringLiteral(handle_escape);
  }

  str += '\0';

  return std::make_shared<Constant>(
      tok, ArrayType::Get(IntegerType::Get(8), std::size(str)), str);
}

}  // namespace kcc