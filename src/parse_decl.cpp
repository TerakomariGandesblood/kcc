//
// Created by kaiser on 2019/12/13.
//

#include "parse.h"

#include <algorithm>
#include <cassert>

#include "calc.h"
#include "error.h"

namespace kcc {

/*
 * Decl
 */
CompoundStmt *Parser::ParseDecl(bool maybe_func_def) {
  if (Try(Tag::kStaticAssert)) {
    ParseStaticAssertDecl();
    return nullptr;
  } else {
    std::uint32_t storage_class_spec{}, func_spec{};
    std::int32_t align{};
    auto base_type{ParseDeclSpec(&storage_class_spec, &func_spec, &align)};

    if (Try(Tag::kSemicolon)) {
      return nullptr;
    } else {
      if (maybe_func_def) {
        return ParseInitDeclaratorList(base_type, storage_class_spec, func_spec,
                                       align);
      } else {
        auto ret{ParseInitDeclaratorList(base_type, storage_class_spec,
                                         func_spec, align)};
        Expect(Tag::kSemicolon);
        return ret;
      }
    }
  }
}

void Parser::ParseStaticAssertDecl() {
  Expect(Tag::kLeftParen);
  auto expr{ParseConstantExpr()};
  Expect(Tag::kComma);

  auto msg{ParseStringLiteral(false)->GetStr()};
  Expect(Tag::kRightParen);
  Expect(Tag::kSemicolon);

  if (!*CalcConstantExpr{}.CalcInteger(expr)) {
    Error(expr, "static_assert failed \"{}\"", msg);
  }
}

/*
 * Decl Spec
 */
QualType Parser::ParseDeclSpec(std::uint32_t *storage_class_spec,
                               std::uint32_t *func_spec, std::int32_t *align) {
#define CHECK_AND_SET_STORAGE_CLASS_SPEC(spec)                  \
  if (*storage_class_spec != 0) {                               \
    Error(tok, "duplicated storage class specifier");           \
  } else if (!storage_class_spec) {                             \
    Error(tok, "storage class specifier are not allowed here"); \
  }                                                             \
  *storage_class_spec |= spec;

#define CHECK_AND_SET_FUNC_SPEC(spec)                                   \
  if (*func_spec & spec) {                                              \
    Warning(tok, "duplicate function specifier declaration specifier"); \
  } else if (!func_spec) {                                              \
    Error(tok, "function specifiers are not allowed here");             \
  }                                                                     \
  *func_spec |= spec;

#define ERROR Error(tok, "two or more data types in declaration specifiers");

#define TYPEOF_CHECK                                              \
  if (has_typeof) {                                               \
    Error(tok, "It is not allowed to use type specifiers here."); \
  }

  std::uint32_t type_spec{}, type_qual{};
  bool has_typeof{false};

  Token tok;
  QualType type;

  while (true) {
    TryParseAttributeSpec();

    tok = Next();

    switch (tok.GetTag()) {
      // GNU 扩展
      case Tag::kExtension:
        break;
      case Tag::kTypeof:
        if (type_spec != 0) {
          Error(tok, "It is not allowed to use typeof here.");
        }
        type = ParseTypeof();
        has_typeof = true;
        break;

        // Storage Class Specifier, 至多有一个
      case Tag::kTypedef:
        CHECK_AND_SET_STORAGE_CLASS_SPEC(kTypedef) break;
      case Tag::kExtern:
        CHECK_AND_SET_STORAGE_CLASS_SPEC(kExtern) break;
      case Tag::kStatic:
        CHECK_AND_SET_STORAGE_CLASS_SPEC(kStatic) break;
      case Tag::kAuto:
        CHECK_AND_SET_STORAGE_CLASS_SPEC(kAuto) break;
      case Tag::kRegister:
        CHECK_AND_SET_STORAGE_CLASS_SPEC(kRegister) break;
      case Tag::kThreadLocal:
        Error(tok, "Does not support _Thread_local");

        // Type specifier
      case Tag::kVoid:
        if (type_spec) {
          ERROR
        }
        TYPEOF_CHECK type_spec |= kVoid;
        break;
      case Tag::kChar:
        if (type_spec & ~kCompChar) {
          ERROR
        }
        TYPEOF_CHECK type_spec |= kChar;
        break;
      case Tag::kShort:
        if (type_spec & ~kCompShort) {
          ERROR
        }
        TYPEOF_CHECK type_spec |= kShort;
        break;
      case Tag::kInt:
        if (type_spec & ~kCompInt) {
          ERROR
        }
        TYPEOF_CHECK type_spec |= kInt;
        break;
      case Tag::kLong:
        if (type_spec & ~kCompLong) {
          ERROR
        }
        TYPEOF_CHECK if (type_spec & kLong) {
          type_spec &= ~kLong;
          type_spec |= kLongLong;
        }
        else {
          type_spec |= kLong;
        }
        break;
      case Tag::kFloat:
        if (type_spec) {
          ERROR
        }
        TYPEOF_CHECK type_spec |= kFloat;
        break;
      case Tag::kDouble:
        if (type_spec & ~kCompDouble) {
          ERROR
        }
        TYPEOF_CHECK type_spec |= kDouble;
        break;
      case Tag::kSigned:
        if (type_spec & ~kCompSigned) {
          ERROR
        }
        TYPEOF_CHECK type_spec |= kSigned;
        break;
      case Tag::kUnsigned:
        if (type_spec & ~kCompUnsigned) {
          ERROR
        }
        TYPEOF_CHECK type_spec |= kUnsigned;
        break;
      case Tag::kBool:
        if (type_spec) {
          ERROR
        }
        TYPEOF_CHECK type_spec |= kBool;
        break;
      case Tag::kStruct:
      case Tag::kUnion:
        if (type_spec) {
          ERROR
        }
        TYPEOF_CHECK type = ParseStructUnionSpec(tok.GetTag() == Tag::kStruct);
        type_spec |= kStructUnionSpec;
        break;
      case Tag::kEnum:
        if (type_spec) {
          ERROR
        }
        TYPEOF_CHECK type = ParseEnumSpec();
        type_spec |= kEnumSpec;
        break;
      case Tag::kComplex:
        TYPEOF_CHECK Error(tok, "Does not support _Complex");
      case Tag::kAtomic:
        TYPEOF_CHECK Error(tok, "Does not support _Atomic");

        // Type qualifier
      case Tag::kConst:
        type_qual |= kConst;
        break;
      case Tag::kRestrict:
        type_qual |= kRestrict;
        break;
      case Tag::kVolatile:
        type_qual |= kVolatile;
        break;

        // Function specifier
      case Tag::kInline:
        CHECK_AND_SET_FUNC_SPEC(kInline) break;
      case Tag::kNoreturn:
        CHECK_AND_SET_FUNC_SPEC(kNoreturn) break;

      case Tag::kAlignas:
        if (!align) {
          Error(tok, "_Alignas are not allowed here");
        }
        *align = std::max(ParseAlignas(), *align);
        break;

      default: {
        if (type_spec == 0 && IsTypeName(tok)) {
          auto ident{scope_->FindUsual(tok.GetIdentifier())};
          type = ident->GetQualType();
          type_spec |= kTypedefName;

          //  typedef int A[];
          //  A a = {1, 2};
          //  A b = {3, 4, 5};
          // 防止类型被修改
          if (type->IsArrayTy() && !type->IsComplete()) {
            type = ArrayType::Get(type->ArrayGetElementType(), {});
          }
        } else {
          goto finish;
        }
      }
    }
  }

finish:
  PutBack();

  TryParseAttributeSpec();

  switch (type_spec) {
    case 0:
      if (!has_typeof) {
        Error(tok, "type specifier missing: {}", tok.GetStr());
      }
      break;
    case kVoid:
      type = VoidType::Get();
      break;
    case kStructUnionSpec:
    case kEnumSpec:
    case kTypedefName:
      break;
    default:
      type = ArithmeticType::Get(type_spec);
  }

  return QualType{type.GetType(), type.GetTypeQual() | type_qual};

#undef CHECK_AND_SET_STORAGE_CLASS_SPEC
#undef CHECK_AND_SET_FUNC_SPEC
#undef ERROR
#undef TYPEOF_CHECK
}

Type *Parser::ParseStructUnionSpec(bool is_struct) {
  TryParseAttributeSpec();

  auto tok{Peek()};
  std::string tag_name;

  if (Try(Tag::kIdentifier)) {
    tag_name = tok.GetIdentifier();
    // 定义
    if (Try(Tag::kLeftBrace)) {
      auto tag{scope_->FindTagInCurrScope(tag_name)};
      // 无前向声明
      if (!tag) {
        auto type{StructType::Get(is_struct, tag_name, scope_)};
        auto ident{MakeAstNode<IdentifierExpr>(tok, tag_name, type)};
        scope_->InsertTag(ident);

        ParseStructDeclList(type);
        Expect(Tag::kRightBrace);
        return type;
      } else {
        if (tag->GetType()->IsComplete()) {
          Error(tok, "redefinition struct or union :{}", tag_name);
        } else {
          ParseStructDeclList(dynamic_cast<StructType *>(tag->GetType()));

          Expect(Tag::kRightBrace);
          return tag->GetType();
        }
      }
    } else {
      // 可能是前向声明或普通的声明
      auto tag{scope_->FindTag(tag_name)};

      if (tag) {
        return tag->GetType();
      } else {
        auto type{StructType::Get(is_struct, tag_name, scope_)};
        auto ident{MakeAstNode<IdentifierExpr>(tok, tag_name, type)};
        scope_->InsertTag(ident);
        return type;
      }
    }
  } else {
    // 无标识符只能是定义
    Expect(Tag::kLeftBrace);

    auto type{StructType::Get(is_struct, "", scope_)};
    ParseStructDeclList(type);

    Expect(Tag::kRightBrace);
    return type;
  }
}

void Parser::ParseStructDeclList(StructType *type) {
  assert(!type->IsComplete());

  auto scope_backup{scope_};
  scope_ = type->GetScope();

  while (!Test(Tag::kRightBrace)) {
    if (Try(Tag::kStaticAssert)) {
      ParseStaticAssertDecl();
    } else {
      std::int32_t align{};
      auto base_type{ParseDeclSpec(nullptr, nullptr, &align)};

      do {
        Token tok;
        auto copy{base_type};

        // 将 bool 类型表示为 int8
        if (copy->IsBoolTy()) {
          copy = QualType{ArithmeticType::Get(kChar | kUnsigned),
                          copy.GetTypeQual()};
        }

        ParseDeclarator(tok, copy);

        TryParseAttributeSpec();

        // 位域
        if (Try(Tag::kColon)) {
          ParseBitField(type, tok, copy);
          continue;
        }

        // struct A {
        //  int a;
        //  struct {
        //    int c;
        //  };
        //};
        if (std::empty(tok.GetStr())) {
          // 此时该 struct / union 不能有名字
          if (copy->IsStructOrUnionTy() && !copy->StructHasName()) {
            auto anonymous{MakeAstNode<ObjectExpr>(tok, "", copy, 0,
                                                   Linkage::kNone, true)};
            type->MergeAnonymous(anonymous);
            continue;
          } else {
            Error(Peek(), "declaration does not declare anything");
          }
        } else {
          auto name{tok.GetIdentifier()};

          if (type->GetMember(name)) {
            Error(Peek(), "duplicate member: '{}'", name);
          } else if (copy->IsArrayTy() && !copy->IsComplete()) {
            // 可能是柔性数组
            // 若结构体定义了至少一个具名成员,
            // 则额外声明其最后成员拥有不完整的数组类型
            if (type->IsStruct() && std::size(type->GetMembers()) > 0) {
              auto member{MakeAstNode<ObjectExpr>(tok, name, copy)};
              type->AddMember(member);
              Expect(Tag::kSemicolon);

              goto finalize;
            } else {
              Error(Peek(), "field '{}' has incomplete type", name);
            }
          } else if (copy->IsFunctionTy()) {
            Error(Peek(), "field '{}' declared as a function", name);
          } else {
            auto member{MakeAstNode<ObjectExpr>(tok, name, copy)};
            type->AddMember(member);
          }
        }
      } while (Try(Tag::kComma));

      Expect(Tag::kSemicolon);
    }
  }

finalize:
  TryParseAttributeSpec();

  type->SetComplete(true);

  // struct / union 中的 tag 的作用域与该 struct / union 所在的作用域相同
  for (const auto &[name, tag] : scope_->AllTagInCurrScope()) {
    if (scope_backup->FindTagInCurrScope(name)) {
      Error(tag->GetLoc(), "redefinition of tag {}", tag->GetName());
    } else {
      scope_backup->InsertTag(name, tag);
    }
  }

  scope_ = scope_backup;
}

void Parser::ParseBitField(StructType *type, const Token &tok,
                           QualType member_type) {
  // 标准中定义位域可以下列拥有四种类型之一
  // int / signed int / unsigned int / _Bool
  // 其他类型是实现定义的, 这里不支持其他类型
  // 注意已经将 bool 表示为 int8
  if (!member_type->IsIntTy() && !member_type->IsCharacterTy()) {
    Error(Peek(), "expect int or bool type for bitfield but got ('{}')",
          member_type.ToString());
  }

  auto expr{ParseConstantExpr()};
  auto width{*CalcConstantExpr{}.CalcInteger(expr)};

  if (width < 0) {
    Error(expr, "expect non negative value");
  } else if (width == 0 && !std::empty(tok.GetStr())) {
    Error(tok, "no declarator expected for a bitfield with width 0");
  } else if ((width > member_type->GetWidth() * 8) ||
             (member_type->IsBoolTy() && width > 1)) {
    Error(expr, "width exceeds its type");
  }

  ObjectExpr *bit_field;
  if (std::empty(tok.GetStr())) {
    bit_field = MakeAstNode<ObjectExpr>(tok, "", member_type, 0, Linkage::kNone,
                                        true, width);
  } else {
    auto name{tok.GetIdentifier()};

    if (type->GetMember(name)) {
      Error(tok, "duplicate member: '{}'", name);
    }

    bit_field = MakeAstNode<ObjectExpr>(tok, name, member_type, 0,
                                        Linkage::kNone, false, width);
  }

  type->AddBitField(bit_field);
}

Type *Parser::ParseEnumSpec() {
  TryParseAttributeSpec();

  std::string tag_name;
  auto tok{Peek()};

  if (Try(Tag::kIdentifier)) {
    tag_name = tok.GetIdentifier();
    // 定义
    if (Try(Tag::kLeftBrace)) {
      auto tag{scope_->FindTagInCurrScope(tag_name)};

      if (!tag) {
        auto type{ArithmeticType::Get(32)};
        auto ident{MakeAstNode<IdentifierExpr>(tok, tag_name, type)};
        scope_->InsertTag(tag_name, ident);
        ParseEnumerator();

        Expect(Tag::kRightBrace);
        return type;
      } else {
        // 不允许前向声明，如果当前作用域中有 tag 则就是重定义
        Error(tok, "redefinition of enumeration tag: {}", tag_name);
      }
    } else {
      // 只能是普通声明
      auto tag{scope_->FindTag(tag_name)};
      if (tag) {
        return tag->GetType();
      } else {
        Error(tok, "unknown enumeration: {}", tag_name);
      }
    }
  } else {
    Expect(Tag::kLeftBrace);
    ParseEnumerator();
    Expect(Tag::kRightBrace);
    return ArithmeticType::Get(32);
  }
}

void Parser::ParseEnumerator() {
  std::int32_t val{};

  do {
    auto tok{Expect(Tag::kIdentifier)};
    TryParseAttributeSpec();

    auto name{tok.GetIdentifier()};
    auto ident{scope_->FindUsualInCurrScope(name)};

    if (ident) {
      Error(tok, "redefinition of enumerator '{}'", name);
    }

    if (Try(Tag::kEqual)) {
      auto expr{ParseConstantExpr()};
      val = *CalcConstantExpr{}.CalcInteger(expr);
    }

    auto enumer{MakeAstNode<EnumeratorExpr>(tok, tok.GetIdentifier(), val)};
    ++val;
    scope_->InsertUsual(name, enumer);

    Try(Tag::kComma);
  } while (!Test(Tag::kRightBrace));
}

std::int32_t Parser::ParseAlignas() {
  Expect(Tag::kLeftParen);

  std::int32_t align{};
  auto tok{Peek()};

  if (IsTypeName(tok)) {
    auto type{ParseTypeName()};
    align = type->GetAlign();
  } else {
    auto expr{ParseConstantExpr()};
    align = *CalcConstantExpr{}.CalcInteger(expr);
  }

  Expect(Tag::kRightParen);

  if (align < 0 || ((align - 1) & align)) {
    Error(tok, "requested alignment is not a power of 2");
  }

  return align;
}

/*
 * Declarator
 */
CompoundStmt *Parser::ParseInitDeclaratorList(QualType &base_type,
                                              std::uint32_t storage_class_spec,
                                              std::uint32_t func_spec,
                                              std::int32_t align) {
  auto stmts{MakeAstNode<CompoundStmt>(Peek())};

  do {
    auto copy{base_type};
    stmts->AddStmt(
        ParseInitDeclarator(copy, storage_class_spec, func_spec, align));
    TryParseAttributeSpec();
  } while (Try(Tag::kComma));

  return stmts;
}

Declaration *Parser::ParseInitDeclarator(QualType &base_type,
                                         std::uint32_t storage_class_spec,
                                         std::uint32_t func_spec,
                                         std::int32_t align) {
  auto token{Peek()};
  Token tok;
  ParseDeclarator(tok, base_type);

  if (std::empty(tok.GetStr())) {
    Error(token, "expect identifier");
  }

  auto decl{
      MakeDeclaration(tok, base_type, storage_class_spec, func_spec, align)};

  if (decl && decl->IsObjDecl()) {
    if (Try(Tag::kEqual)) {
      if (!scope_->IsFileScope() &&
          !(scope_->IsBlockScope() && storage_class_spec & kStatic)) {
        ParseInitDeclaratorSub(decl);
      } else {
        decl->SetConstant(
            ParseConstantInitializer(decl->GetIdent()->GetType(), false, true));
      }
    }
  }

  return decl;
}

void Parser::ParseInitDeclaratorSub(Declaration *decl) {
  auto ident{decl->GetIdent()};

  if (!scope_->IsFileScope() && ident->GetLinkage() == Linkage::kExternal) {
    Error(ident->GetLoc(), "{} has both 'extern' and initializer",
          ident->GetName());
  }

  if (!ident->GetType()->IsComplete() && !ident->GetType()->IsArrayTy()) {
    Error(ident->GetLoc(), "variable '{}' has initializer but incomplete type",
          ident->GetName());
  }

  std::vector<Initializer> inits;
  if (auto constant{ParseInitializer(inits, ident->GetType(), false, true)}) {
    decl->SetConstant(constant);
  } else {
    decl->AddInits(inits);
  }
}

void Parser::ParseDeclarator(Token &tok, QualType &base_type) {
  ParsePointer(base_type);
  ParseDirectDeclarator(tok, base_type);
}

void Parser::ParsePointer(QualType &type) {
  while (Try(Tag::kStar)) {
    type = QualType{PointerType::Get(type), ParseTypeQualList()};
  }
}

std::uint32_t Parser::ParseTypeQualList() {
  std::uint32_t type_qual{};

  auto token{Peek()};
  while (true) {
    if (Try(Tag::kConst)) {
      type_qual |= kConst;
    } else if (Try(Tag::kRestrict)) {
      type_qual |= kRestrict;
    } else if (Try(Tag::kVolatile)) {
      type_qual |= kVolatile;
    } else if (Try(Tag::kAtomic)) {
      Error(token, "Does not support _Atomic");
    } else {
      break;
    }
    token = Peek();
  }

  return type_qual;
}

void Parser::ParseDirectDeclarator(Token &tok, QualType &base_type) {
  if (Test(Tag::kIdentifier)) {
    tok = Next();
    ParseDirectDeclaratorTail(base_type);
  } else if (Try(Tag::kLeftParen)) {
    auto begin{index_};
    auto temp{QualType{ArithmeticType::Get(kInt)}};
    // 此时的 base_type 不一定是正确的, 先跳过括号中的内容
    ParseDeclarator(tok, temp);
    Expect(Tag::kRightParen);

    ParseDirectDeclaratorTail(base_type);
    auto end{index_};

    index_ = begin;
    ParseDeclarator(tok, base_type);
    Expect(Tag::kRightParen);
    index_ = end;
  } else {
    ParseDirectDeclaratorTail(base_type);
  }
}

void Parser::ParseDirectDeclaratorTail(QualType &base_type) {
  if (Try(Tag::kLeftSquare)) {
    if (base_type->IsFunctionTy()) {
      Error(Peek(), "the element of array cannot be a function");
    }

    auto len{ParseArrayLength()};
    Expect(Tag::kRightSquare);

    ParseDirectDeclaratorTail(base_type);

    if (!base_type->IsComplete()) {
      Error(Peek(), "has incomplete element type");
    }

    base_type = ArrayType::Get(base_type, len);
  } else if (Try(Tag::kLeftParen)) {
    if (base_type->IsFunctionTy()) {
      Error(Peek(), "the return value of function cannot be function");
    } else if (base_type->IsArrayTy()) {
      Error(Peek(), "the return value of function cannot be array");
    }

    EnterProto();
    auto [params, var_args]{ParseParamTypeList()};
    ExitProto();

    Expect(Tag::kRightParen);

    ParseDirectDeclaratorTail(base_type);

    base_type = FunctionType::Get(base_type, params, var_args);
  }
}

std::optional<std::size_t> Parser::ParseArrayLength() {
  // 忽略掉
  while (true) {
    if (Try(Tag::kTypedef) || Try(Tag::kExtern) || Try(Tag::kStatic) ||
        Try(Tag::kThreadLocal) || Try(Tag::kAuto) || Try(Tag::kRegister)) {
    } else if (ParseTypeQualList()) {
    } else {
      break;
    }
  }

  if (Test(Tag::kRightSquare)) {
    return {};
  }

  auto expr{ParseAssignExpr()};

  if (!expr->GetQualType()->IsIntegerTy()) {
    Error(expr, "The array size must be an integer: '{}'",
          expr->GetType()->ToString());
  }

  auto len{CalcConstantExpr{}.CalcInteger(expr, false)};

  if (!len) {
    Error(Peek(), "not support VLA");
  } else if (*len < 0) {
    Error(Peek(), "Array size must be greater than zero: '{}'", *len);
  }

  return *len;
}

std::pair<std::vector<ObjectExpr *>, bool> Parser::ParseParamTypeList() {
  if (Test(Tag::kRightParen)) {
    return {{}, false};
  }

  auto param{ParseParamDecl()};
  if (param->GetType()->IsVoidTy()) {
    return {{}, false};
  }

  std::vector<ObjectExpr *> params;
  params.push_back(param);

  while (Try(Tag::kComma)) {
    if (Try(Tag::kEllipsis)) {
      return {params, true};
    }

    param = ParseParamDecl();
    if (param->GetType()->IsVoidTy()) {
      Error(param->GetLoc(),
            "'void' must be the first and only parameter if specified");
    }
    params.push_back(param);
  }

  return {params, false};
}

ObjectExpr *Parser::ParseParamDecl() {
  auto base_type{ParseDeclSpec(nullptr, nullptr, nullptr)};

  Token tok;
  ParseDeclarator(tok, base_type);

  base_type = Type::MayCast(base_type);

  if (std::empty(tok.GetStr())) {
    return MakeAstNode<ObjectExpr>(tok, "", base_type, 0, Linkage::kNone, true);
  }

  auto decl{MakeDeclaration(tok, base_type, 0, 0, 0)};
  auto obj{decl->GetIdent()->ToObjectExpr()};
  obj->SetDecl(decl);

  return obj;
}

/*
 * type name
 */
QualType Parser::ParseTypeName() {
  auto base_type{ParseDeclSpec(nullptr, nullptr, nullptr)};
  ParseAbstractDeclarator(base_type);
  return base_type;
}

void Parser::ParseAbstractDeclarator(QualType &type) {
  ParsePointer(type);
  ParseDirectAbstractDeclarator(type);
}

void Parser::ParseDirectAbstractDeclarator(QualType &type) {
  Token tok;
  ParseDirectDeclarator(tok, type);

  if (!std::empty(tok.GetStr())) {
    Error(tok, "unexpected identifier '{}'", tok.GetStr());
  }
}

}  // namespace kcc
