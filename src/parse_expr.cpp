//
// Created by kaiser on 2019/12/13.
//

#include "parse.h"

#include <cassert>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "encoding.h"
#include "error.h"
#include "lex.h"

namespace kcc {

/*
 * Expr
 */
Expr *Parser::ParseExpr() {
  // GNU 扩展, 当使用 -ansi 时避免警告
  Try(Tag::kExtension);

  auto lhs{ParseAssignExpr()};

  auto token{Peek()};
  while (Try(Tag::kComma)) {
    auto rhs{ParseAssignExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kComma, lhs, rhs);
    token = Peek();
  }

  return lhs;
}

Expr *Parser::ParseAssignExpr() {
  Try(Tag::kExtension);

  auto lhs{ParseConditionExpr()};
  Expr *rhs;

  auto token{Next()};
  switch (token.GetTag()) {
  case Tag::kEqual:
    rhs = ParseAssignExpr();
    break;
  case Tag::kStarEqual:
    rhs = ParseAssignExpr();
    rhs = MakeAstNode<BinaryOpExpr>(token, Tag::kStar, lhs, rhs);
    break;
  case Tag::kSlashEqual:
    rhs = ParseAssignExpr();
    rhs = MakeAstNode<BinaryOpExpr>(token, Tag::kSlash, lhs, rhs);
    break;
  case Tag::kPercentEqual:
    rhs = ParseAssignExpr();
    rhs = MakeAstNode<BinaryOpExpr>(token, Tag::kPercent, lhs, rhs);
    break;
  case Tag::kPlusEqual:
    rhs = ParseAssignExpr();
    rhs = MakeAstNode<BinaryOpExpr>(token, Tag::kPlus, lhs, rhs);
    break;
  case Tag::kMinusEqual:
    rhs = ParseAssignExpr();
    rhs = MakeAstNode<BinaryOpExpr>(token, Tag::kMinus, lhs, rhs);
    break;
  case Tag::kLessLessEqual:
    rhs = ParseAssignExpr();
    rhs = MakeAstNode<BinaryOpExpr>(token, Tag::kLessLess, lhs, rhs);
    break;
  case Tag::kGreaterGreaterEqual:
    rhs = ParseAssignExpr();
    rhs = MakeAstNode<BinaryOpExpr>(token, Tag::kGreaterGreater, lhs, rhs);
    break;
  case Tag::kAmpEqual:
    rhs = ParseAssignExpr();
    rhs = MakeAstNode<BinaryOpExpr>(token, Tag::kAmp, lhs, rhs);
    break;
  case Tag::kCaretEqual:
    rhs = ParseAssignExpr();
    rhs = MakeAstNode<BinaryOpExpr>(token, Tag::kCaret, lhs, rhs);
    break;
  case Tag::kPipeEqual:
    rhs = ParseAssignExpr();
    rhs = MakeAstNode<BinaryOpExpr>(token, Tag::kPipe, lhs, rhs);
    break;
  default: {
    PutBack();
    return lhs;
  }
  }

  return MakeAstNode<BinaryOpExpr>(token, Tag::kEqual, lhs, rhs);
}

Expr *Parser::ParseConditionExpr() {
  auto cond{ParseLogicalOrExpr()};

  auto token{Peek()};
  if (Try(Tag::kQuestion)) {
    // GNU 扩展
    // a ?: b 相当于 a ? a: c
    auto lhs{Test(Tag::kColon) ? cond : ParseExpr()};
    Expect(Tag::kColon);
    auto rhs{ParseConditionExpr()};

    return MakeAstNode<ConditionOpExpr>(token, cond, lhs, rhs);
  }

  return cond;
}

Expr *Parser::ParseLogicalOrExpr() {
  auto lhs{ParseLogicalAndExpr()};

  auto token{Peek()};
  while (Try(Tag::kPipePipe)) {
    auto rhs{ParseLogicalAndExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kPipePipe, lhs, rhs);
    token = Peek();
  }

  return lhs;
}

Expr *Parser::ParseLogicalAndExpr() {
  auto lhs{ParseInclusiveOrExpr()};

  auto token{Peek()};
  while (Try(Tag::kAmpAmp)) {
    auto rhs{ParseInclusiveOrExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kAmpAmp, lhs, rhs);
    token = Peek();
  }

  return lhs;
}

Expr *Parser::ParseInclusiveOrExpr() {
  auto lhs{ParseExclusiveOrExpr()};

  auto token{Peek()};
  while (Try(Tag::kPipe)) {
    auto rhs{ParseExclusiveOrExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kPipe, lhs, rhs);
    token = Peek();
  }

  return lhs;
}

Expr *Parser::ParseExclusiveOrExpr() {
  auto lhs{ParseAndExpr()};

  auto token{Peek()};
  while (Try(Tag::kCaret)) {
    auto rhs{ParseAndExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kCaret, lhs, rhs);
    token = Peek();
  }

  return lhs;
}

Expr *Parser::ParseAndExpr() {
  auto lhs{ParseEqualityExpr()};

  auto token{Peek()};
  while (Try(Tag::kAmp)) {
    auto rhs{ParseEqualityExpr()};
    lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kAmp, lhs, rhs);
    token = Peek();
  }

  return lhs;
}

Expr *Parser::ParseEqualityExpr() {
  auto lhs{ParseRelationExpr()};

  auto token{Peek()};
  while (true) {
    if (Try(Tag::kEqualEqual)) {
      auto rhs{ParseRelationExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kEqualEqual, lhs, rhs);
    } else if (Try(Tag::kExclaimEqual)) {
      auto rhs{ParseRelationExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kExclaimEqual, lhs, rhs);
    } else {
      break;
    }
    token = Peek();
  }

  return lhs;
}

Expr *Parser::ParseRelationExpr() {
  auto lhs{ParseShiftExpr()};

  auto token{Peek()};
  while (true) {
    if (Try(Tag::kLess)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kLess, lhs, rhs);
    } else if (Try(Tag::kGreater)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kGreater, lhs, rhs);
    } else if (Try(Tag::kLessEqual)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kLessEqual, lhs, rhs);
    } else if (Try(Tag::kGreaterEqual)) {
      auto rhs{ParseShiftExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kGreaterEqual, lhs, rhs);
    } else {
      break;
    }
    token = Peek();
  }

  return lhs;
}

Expr *Parser::ParseShiftExpr() {
  auto lhs{ParseAdditiveExpr()};

  auto token{Peek()};
  while (true) {
    if (Try(Tag::kLessLess)) {
      auto rhs{ParseAdditiveExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kLessLess, lhs, rhs);
    } else if (Try(Tag::kGreaterGreater)) {
      auto rhs{ParseAdditiveExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kGreaterGreater, lhs, rhs);
    } else {
      break;
    }
    token = Peek();
  }

  return lhs;
}

Expr *Parser::ParseAdditiveExpr() {
  auto lhs{ParseMultiplicativeExpr()};

  auto token{Peek()};
  while (true) {
    if (Try(Tag::kPlus)) {
      auto rhs{ParseMultiplicativeExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kPlus, lhs, rhs);
    } else if (Try(Tag::kMinus)) {
      auto rhs{ParseMultiplicativeExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kMinus, lhs, rhs);
    } else {
      break;
    }
    token = Peek();
  }

  return lhs;
}

Expr *Parser::ParseMultiplicativeExpr() {
  auto lhs{ParseCastExpr()};

  auto token{Peek()};
  while (true) {
    if (Try(Tag::kStar)) {
      auto rhs{ParseCastExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kStar, lhs, rhs);
    } else if (Try(Tag::kSlash)) {
      auto rhs{ParseCastExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kSlash, lhs, rhs);
    } else if (Try(Tag::kPercent)) {
      auto rhs{ParseCastExpr()};
      lhs = MakeAstNode<BinaryOpExpr>(token, Tag::kPercent, lhs, rhs);
    } else {
      break;
    }
    token = Peek();
  }

  return lhs;
}

Expr *Parser::ParseCastExpr() {
  if (Try(Tag::kLeftParen)) {
    if (IsTypeName(Peek())) {
      auto type{ParseTypeName()};
      Expect(Tag::kRightParen);

      // 复合字面量
      if (Test(Tag::kLeftBrace)) {
        return ParsePostfixExprTail(ParseCompoundLiteral(type));
      } else {
        return MakeAstNode<TypeCastExpr>(Peek(), ParseCastExpr(), type);
      }
    } else {
      PutBack();
      return ParseUnaryExpr();
    }
  } else {
    return ParseUnaryExpr();
  }
}

Expr *Parser::ParseUnaryExpr() {
  auto token{Next()};
  switch (token.GetTag()) {
  // 默认为前缀
  case Tag::kPlusPlus:
    return MakeAstNode<UnaryOpExpr>(token, Tag::kPlusPlus, ParseUnaryExpr());
  case Tag::kMinusMinus:
    return MakeAstNode<UnaryOpExpr>(token, Tag::kMinusMinus, ParseUnaryExpr());
  case Tag::kAmp:
    return MakeAstNode<UnaryOpExpr>(token, Tag::kAmp, ParseCastExpr());
  case Tag::kStar:
    return MakeAstNode<UnaryOpExpr>(token, Tag::kStar, ParseCastExpr());
  case Tag::kPlus:
    return MakeAstNode<UnaryOpExpr>(token, Tag::kPlus, ParseCastExpr());
  case Tag::kMinus:
    return MakeAstNode<UnaryOpExpr>(token, Tag::kMinus, ParseCastExpr());
  case Tag::kTilde:
    return MakeAstNode<UnaryOpExpr>(token, Tag::kTilde, ParseCastExpr());
  case Tag::kExclaim:
    return MakeAstNode<UnaryOpExpr>(token, Tag::kExclaim, ParseCastExpr());
  case Tag::kSizeof:
    return ParseSizeof();
  case Tag::kAlignof:
    return ParseAlignof();
  case Tag::kOffsetof:
    return ParseOffsetof();
  case Tag::kTypeid:
    return ParseTypeid();
  default:
    PutBack();
    return ParsePostfixExpr();
  }
}

Expr *Parser::ParseSizeof() {
  QualType type;

  auto token{Peek()};
  if (Try(Tag::kLeftParen)) {
    if (!IsTypeName(Peek())) {
      auto expr{ParseExpr()};
      type = expr->GetType();
    } else {
      type = ParseTypeName();
    }
    Expect(Tag::kRightParen);
  } else {
    auto expr{ParseUnaryExpr()};
    type = expr->GetType();
  }

  if (!type->IsComplete() && !type->IsVoidTy() && !type->IsFunctionTy()) {
    Error(token, "sizeof(incomplete type)");
  }

  return MakeAstNode<ConstantExpr>(
      token, ArithmeticType::Get(kLong | kUnsigned),
      static_cast<std::uint64_t>(type->GetWidth()));
}

Expr *Parser::ParseAlignof() {
  QualType type;

  Expect(Tag::kLeftParen);

  auto token{Peek()};
  if (!IsTypeName(token)) {
    Error(token, "expect type name");
  }

  type = ParseTypeName();
  Expect(Tag::kRightParen);

  return MakeAstNode<ConstantExpr>(
      token, ArithmeticType::Get(kLong | kUnsigned),
      static_cast<std::uint64_t>(type->GetAlign()));
}

Expr *Parser::ParsePostfixExpr() {
  auto expr{TryParseCompoundLiteral()};

  if (expr) {
    return ParsePostfixExprTail(expr);
  } else {
    return ParsePostfixExprTail(ParsePrimaryExpr());
  }
}

Expr *Parser::TryParseCompoundLiteral() {
  auto begin{index_};

  if (Try(Tag::kLeftParen) && IsTypeName(Peek())) {
    auto type{ParseTypeName()};

    if (Try(Tag::kRightParen) && Test(Tag::kLeftBrace)) {
      return ParseCompoundLiteral(type);
    }
  }

  index_ = begin;
  return nullptr;
}

Expr *Parser::ParseCompoundLiteral(QualType type) {
  if (scope_->IsFileScope()) {
    auto obj{
        MakeAstNode<ObjectExpr>(Peek(), "", type, 0, Linkage::kInternal, true)};
    auto decl{MakeAstNode<Declaration>(Peek(), obj)};

    decl->SetConstant(
        ParseConstantInitializer(decl->GetIdent()->GetType(), false, true));

    obj->SetDecl(decl);

    return obj;
  } else {
    auto obj{MakeAstNode<ObjectExpr>(Peek(), ".compoundliteral", type, 0,
                                     Linkage::kNone, true)};
    auto decl{MakeAstNode<Declaration>(Peek(), obj)};

    ParseInitDeclaratorSub(decl);
    compound_stmt_.top()->AddStmt(decl);

    return obj;
  }
}

Expr *Parser::ParsePostfixExprTail(Expr *expr) {
  auto token{Peek()};
  while (true) {
    switch (Next().GetTag()) {
    case Tag::kLeftSquare:
      expr = ParseIndexExpr(expr);
      break;
    case Tag::kLeftParen:
      expr = ParseFuncCallExpr(expr);
      break;
    case Tag::kArrow:
      expr = MakeAstNode<UnaryOpExpr>(token, Tag::kStar, expr);
      expr = ParseMemberRefExpr(expr);
      break;
    case Tag::kPeriod:
      expr = ParseMemberRefExpr(expr);
      break;
    case Tag::kPlusPlus:
      expr = MakeAstNode<UnaryOpExpr>(token, Tag::kPostfixPlusPlus, expr);
      break;
    case Tag::kMinusMinus:
      expr = MakeAstNode<UnaryOpExpr>(token, Tag::kPostfixMinusMinus, expr);
      break;
    default:
      PutBack();
      return expr;
    }
    token = Peek();
  }
}

Expr *Parser::ParseIndexExpr(Expr *expr) {
  auto token{Peek()};
  auto rhs{ParseExpr()};
  Expect(Tag::kRightSquare);

  return MakeAstNode<UnaryOpExpr>(
      token, Tag::kStar,
      MakeAstNode<BinaryOpExpr>(token, Tag::kPlus, expr, rhs));
}

Expr *Parser::ParseFuncCallExpr(Expr *expr) {
  std::vector<Expr *> args;
  auto loc{Peek().GetLoc()};

  if (expr->GetType()->IsFunctionTy() &&
      expr->GetType()->FuncGetName() == "__builtin_va_arg_sub") {
    args.push_back(ParseAssignExpr());
    Expect(Tag::kComma);
    auto type{ParseTypeName()};
    Expect(Tag::kRightParen);
    auto ret{MakeAstNode<FuncCallExpr>(loc, expr, args)};
    ret->SetVaArgType(type.GetType());
    return ret;
  }

  while (!Try(Tag::kRightParen)) {
    args.push_back(ParseAssignExpr());

    if (!Test(Tag::kRightParen)) {
      Expect(Tag::kComma);
    }
  }

  return MakeAstNode<FuncCallExpr>(loc, expr, args);
}

Expr *Parser::ParseMemberRefExpr(Expr *expr) {
  auto token{Peek()};

  auto member{Expect(Tag::kIdentifier)};
  auto member_name{member.GetIdentifier()};

  auto type{expr->GetQualType()};
  if (!type->IsStructOrUnionTy()) {
    Error(expr, "an struct/union expected: '{}'", type.ToString());
  }

  auto rhs{type->StructGetMember(member_name)};
  if (!rhs) {
    Error(member, "'{}' is not a member of '{}'", member_name,
          type->StructGetName());
  }

  // 当函数返回一个 struct / union 时
  if (expr->Kind() == AstNodeType::kFuncCallExpr) {
    auto obj{
        MakeAstNode<ObjectExpr>(Peek(), "", type, 0, Linkage::kNone, true)};
    auto decl{MakeAstNode<Declaration>(Peek(), obj)};

    std::vector<Initializer> inits;
    inits.emplace_back(
        type.GetType(), expr,
        std::vector<
            std::tuple<Type *, std::int32_t, std::int32_t, std::int32_t>>{});
    decl->AddInits(inits);

    compound_stmt_.top()->AddStmt(decl);

    expr = obj;
  }

  return MakeAstNode<BinaryOpExpr>(token, Tag::kPeriod, expr, rhs);
}

Expr *Parser::ParsePrimaryExpr() {
  auto token{Peek()};

  if (auto expr{TryParseStmtExpr()}) {
    return expr;
  }

  if (Peek().IsIdentifier()) {
    auto name{Next().GetIdentifier()};
    auto ident{scope_->FindUsual(name)};

    if (ident) {
      return ident;
    } else {
      Error(token, "undefined symbol: {}", name);
    }
  } else if (Peek().IsConstant()) {
    return ParseConstant();
  } else if (Peek().IsStringLiteral()) {
    return ParseStringLiteral();
  } else if (Try(Tag::kLeftParen)) {
    auto expr{ParseExpr()};
    Expect(Tag::kRightParen);
    return expr;
  } else if (Try(Tag::kGeneric)) {
    return ParseGenericSelection();
  } else if (Try(Tag::kFuncName)) {
    if (func_def_ == nullptr) {
      Error(token, "Not allowed to use __func__ or __FUNCTION__ here");
    }
    return MakeAstNode<StringLiteralExpr>(token, func_def_->GetName());
  } else if (Try(Tag::kFuncSignature)) {
    if (func_def_ == nullptr) {
      Error(token, "Not allowed to use __PRETTY_FUNCTION__ here");
    }
    return MakeAstNode<StringLiteralExpr>(
        token, func_def_->GetFuncType()->ToString() + ": " +
                   func_def_->GetFuncType()->FuncGetName());
  } else if (Try(Tag::kHugeVal)) {
    return ParseHugeVal();
  } else if (Try(Tag::kInff)) {
    return ParseInff();
  } else {
    Error(token, "'{}' unexpected", token.GetStr());
  }
}

Expr *Parser::ParseConstant() {
  if (Peek().IsCharacter()) {
    return ParseCharacter();
  } else if (Peek().IsInteger()) {
    return ParseInteger();
  } else if (Peek().IsFloatPoint()) {
    return ParseFloat();
  } else {
    assert(false);
    return nullptr;
  }
}

Expr *Parser::ParseCharacter() {
  auto token{Next()};
  Scanner scanner{token.GetStr(), token.GetLoc()};
  auto [val, encoding]{scanner.HandleCharacter()};

  std::uint32_t type_spec{};
  switch (encoding) {
  case Encoding::kNone:
    val = static_cast<char>(val);
    type_spec = kInt;
    break;
  case Encoding::kChar16:
    val = static_cast<char16_t>(val);
    type_spec = kShort | kUnsigned;
    break;
  case Encoding::kChar32:
    val = static_cast<char32_t>(val);
    type_spec = kInt | kUnsigned;
    break;
  case Encoding::kWchar:
    val = static_cast<wchar_t>(val);
    type_spec = kInt | kUnsigned;
    break;
  case Encoding::kUtf8:
    Error(token, "Can't use u8 here");
  default:
    assert(false);
  }

  return MakeAstNode<ConstantExpr>(token, ArithmeticType::Get(type_spec),
                                   static_cast<std::uint64_t>(val));
}

Expr *Parser::ParseInteger() {
  auto token{Next()};
  auto str{token.GetStr()};
  std::uint64_t val;
  std::size_t end;

  try {
    // GNU 扩展, 也可以有后缀
    if (std::size(str) >= 3 &&
        (str.substr(0, 2) == "0b" || str.substr(0, 2) == "0B")) {
      val = std::stoull(str.substr(2), &end, 2);
      end += 2;
    } else {
      // 当 base 为 0 时，自动检测进制
      val = std::stoull(str, &end, 0);
    }
  } catch (const std::out_of_range &error) {
    Error(token, "integer out of range");
  }

  auto backup{end};
  std::uint32_t type_spec{};
  for (auto ch{str[end]}; ch != '\0'; ch = str[++end]) {
    if (ch == 'u' || ch == 'U') {
      if (type_spec & kUnsigned) {
        Error(token, "invalid suffix: {}", str.substr(backup));
      }
      type_spec |= kUnsigned;
    } else if (ch == 'l' || ch == 'L') {
      if ((type_spec & kLong) || (type_spec & kLongLong)) {
        Error(token, "invalid suffix: {}", str.substr(backup));
      }

      if (str[end + 1] == 'l' || str[end + 1] == 'L') {
        type_spec |= kLongLong;
        ++end;
      } else {
        type_spec |= kLong;
      }
    } else {
      Error(token, "invalid suffix: {}", str.substr(backup));
    }
  }

  // 十进制
  bool decimal{'1' <= str.front() && str.front() <= '9'};

  if (decimal) {
    switch (type_spec) {
    case 0:
      if ((val > static_cast<std::uint64_t>(
                     std::numeric_limits<std::int32_t>::max()))) {
        type_spec |= kLong;
      } else {
        type_spec |= kInt;
      }
      break;
    case kUnsigned:
      if (val > std::numeric_limits<std::uint32_t>::max()) {
        type_spec |= (kLong | kUnsigned);
      } else {
        type_spec |= (kInt | kUnsigned);
      }
      break;
    default:
      break;
    }
  } else {
    switch (type_spec) {
    case 0:
      if (val > static_cast<std::uint64_t>(
                    std::numeric_limits<std::int64_t>::max())) {
        type_spec |= (kLong | kUnsigned);
      } else if (val > std::numeric_limits<std::uint32_t>::max()) {
        type_spec |= kLong;
      } else if (val > static_cast<std::uint64_t>(
                           std::numeric_limits<std::int32_t>::max())) {
        type_spec |= (kInt | kUnsigned);
      } else {
        type_spec |= kInt;
      }
      break;
    case kUnsigned:
      if (val > std::numeric_limits<std::uint32_t>::max()) {
        type_spec |= (kLong | kUnsigned);
      } else {
        type_spec |= (kInt | kUnsigned);
      }
      break;
    case kLong:
      if (val > static_cast<std::uint64_t>(
                    std::numeric_limits<std::int64_t>::max())) {
        type_spec |= (kLong | kUnsigned);
      } else {
        type_spec |= kLong;
      }
      break;
    case kLongLong:
      if (val > static_cast<std::uint64_t>(
                    std::numeric_limits<std::int64_t>::max())) {
        type_spec |= (kLongLong | kUnsigned);
      } else {
        type_spec |= kLongLong;
      }
      break;
    default:
      break;
    }
  }

  return MakeAstNode<ConstantExpr>(token, ArithmeticType::Get(type_spec), val);
}

Expr *Parser::ParseFloat() {
  auto tok{Next()};
  auto str{tok.GetStr()};
  long double val;
  std::size_t end;

  try {
    val = std::stold(str, &end);
  } catch (const std::out_of_range &err) {
    std::istringstream iss{str};
    iss >> val;
    end = iss.tellg();

    // 当值过小时是可以的
    if (std::abs(val) > 1.0) {
      Error(tok, "float point out of range");
    }
  }

  auto backup{end};
  std::uint32_t type_spec{kDouble};
  if (str[end] == 'f' || str[end] == 'F') {
    type_spec = kFloat;
    ++end;
  } else if (str[end] == 'l' || str[end] == 'L') {
    type_spec = kLong | kDouble;
    ++end;
  }

  if (str[end] != '\0') {
    Error(tok, "invalid suffix:{}", str.substr(backup));
  }

  return MakeAstNode<ConstantExpr>(tok, ArithmeticType::Get(type_spec),
                                   str.substr(0, backup));
}

StringLiteralExpr *Parser::ParseStringLiteral(bool handle_escape) {
  auto loc{Peek().GetLoc()};
  // 如果一个没有指定编码而另一个指定了那么可以连接
  // 两个都指定了不能连接
  auto tok{Expect(Tag::kStringLiteral)};
  auto [str, encoding]{
      Scanner{tok.GetStr(), tok.GetLoc()}.HandleStringLiteral(handle_escape)};
  ConvertString(str, encoding);

  while (Test(Tag::kStringLiteral)) {
    tok = Next();
    auto [next_str, next_encoding]{
        Scanner{tok.GetStr()}.HandleStringLiteral(handle_escape)};
    ConvertString(next_str, next_encoding);

    if (encoding == Encoding::kNone && next_encoding != Encoding::kNone) {
      ConvertString(str, next_encoding);
      encoding = next_encoding;
    } else if (encoding != Encoding::kNone &&
               next_encoding == Encoding::kNone) {
      ConvertString(next_str, encoding);
      next_encoding = encoding;
    }

    if (encoding != next_encoding) {
      Error(loc, "cannot concat literal with different encodings");
    }

    str += next_str;
  }

  std::uint32_t type_spec{};

  switch (encoding) {
  case Encoding::kUtf8:
  case Encoding::kNone:
    type_spec = kChar;
    break;
  case Encoding::kChar16:
    type_spec = kShort | kUnsigned;
    break;
  case Encoding::kChar32:
  case Encoding::kWchar:
    type_spec = kInt | kUnsigned;
    break;
  default:
    assert(false);
  }

  return MakeAstNode<StringLiteralExpr>(loc, ArithmeticType::Get(type_spec),
                                        str);
}

Expr *Parser::ParseGenericSelection() {
  Expect(Tag::kLeftParen);
  auto control_expr{ParseAssignExpr()};
  control_expr = Expr::MayCast(control_expr);
  Expect(Tag::kComma);

  Expr *ret{nullptr};
  Expr *default_expr{nullptr};

  auto token{Peek()};
  while (true) {
    if (Try(Tag::kDefault)) {
      if (default_expr) {
        Error(token, "duplicate default generic association");
      }

      Expect(Tag::kColon);
      default_expr = ParseAssignExpr();
    } else {
      auto type{ParseTypeName()};

      if (type->Compatible(control_expr->GetType())) {
        if (ret) {
          Error(token,
                "more than one generic association are compatible with control "
                "expression");
        }

        Expect(Tag::kColon);
        ret = ParseAssignExpr();
      } else {
        Expect(Tag::kColon);
        ParseAssignExpr();
      }
    }

    if (!Try(Tag::kComma)) {
      Expect(Tag::kRightParen);
      break;
    }
  }

  if (!ret && !default_expr) {
    Error(Peek(), "no compatible generic association");
  }

  return ret ? ret : default_expr;
}

Expr *Parser::ParseConstantExpr() { return ParseConditionExpr(); }

} // namespace kcc
