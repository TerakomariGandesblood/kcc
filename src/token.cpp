//
// Created by kaiser on 2019/10/30.
//

#include "token.h"

#include <cassert>

#include <fmt/format.h>
#include <magic_enum.hpp>

#include "lex.h"

namespace kcc {

/*
 * Token
 */
bool Token::TagIs(Tag tag) const { return tag_ == tag; }

void Token::SetTag(Tag tag) { tag_ = tag; }

Tag Token::GetTag() const { return tag_; }

std::string Token::GetStr() const { return str_; }

void Token::SetStr(const std::string &str) { str_ = str; }

std::string Token::GetIdentifier() const {
  assert(IsIdentifier());
  return Scanner{str_}.HandleIdentifier();
}

Location Token::GetLoc() const { return loc_; }

void Token::SetLoc(const Location &loc) { loc_ = loc; }

std::string Token::ToString() const {
  return fmt::format("{:<25}str: {:<25}loc: <{}>", magic_enum::enum_name(tag_),
                     str_, loc_.ToLocStr());
}

bool Token::IsEof() const { return tag_ == Tag::kEof; }

bool Token::IsIdentifier() const { return tag_ == Tag::kIdentifier; }

bool Token::IsStringLiteral() const { return tag_ == Tag::kStringLiteral; }

bool Token::IsConstant() const {
  return IsInteger() || IsFloatPoint() || IsCharacter();
}

bool Token::IsInteger() const { return tag_ == Tag::kInteger; }

bool Token::IsFloatPoint() const { return tag_ == Tag::kFloatingPoint; }

bool Token::IsCharacter() const { return tag_ == Tag::kCharacter; }

bool Token::IsTypeSpecQual() const {
  return tag_ == Tag::kVoid || tag_ == Tag::kChar || tag_ == Tag::kShort ||
         tag_ == Tag::kInt || tag_ == Tag::kLong || tag_ == Tag::kFloat ||
         tag_ == Tag::kDouble || tag_ == Tag::kSigned ||
         tag_ == Tag::kUnsigned || tag_ == Tag::kBool ||
         tag_ == Tag::kComplex || tag_ == Tag::kAtomic ||
         tag_ == Tag::kStruct || tag_ == Tag::kUnion || tag_ == Tag::kEnum ||
         tag_ == Tag::kConst || tag_ == Tag::kRestrict ||
         tag_ == Tag::kVolatile || tag_ == Tag::kAtomic || tag_ == Tag::kTypeof;
}

bool Token::IsDeclSpec() const {
  return IsTypeSpecQual() || tag_ == Tag::kAttribute || tag_ == Tag::kInline ||
         tag_ == Tag::kNoreturn || tag_ == Tag::kAlignas ||
         tag_ == Tag::kStaticAssert || tag_ == Tag::kTypedef ||
         tag_ == Tag::kExtern || tag_ == Tag::kStatic ||
         tag_ == Tag::kThreadLocal || tag_ == Tag::kAuto ||
         tag_ == Tag::kRegister;
}

} // namespace kcc
