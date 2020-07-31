//
// Created by kaiser on 2019/10/30.
//

#pragma once

#include <string>

#include <QMetaEnum>
#include <QObject>
#include <QString>

#include "location.h"

namespace kcc {

class TokenTag : public QObject {
  Q_OBJECT
public:
  enum Values {
    kAuto,
    kBreak,
    kCase,
    kChar,
    kConst,
    kContinue,
    kDefault,
    kDo,
    kDouble,
    kElse,
    kEnum,
    kExtern,
    kFloat,
    kFor,
    kGoto,
    kIf,
    kInline, // C99
    kInt,
    kLong,
    kRegister,
    kRestrict, // C99
    kReturn,
    kShort,
    kSigned,
    kSizeof,
    kStatic,
    kStruct,
    kSwitch,
    kTypedef,
    kUnion,
    kUnsigned,
    kVoid,
    kVolatile,
    kWhile,
    kAlignas,      // C11
    kAlignof,      // C11
    kAtomic,       // C11
    kBool,         // C99
    kComplex,      // C99
    kGeneric,      // C11
    kImaginary,    // C99
    kNoreturn,     // C11
    kStaticAssert, // C11
    kThreadLocal,  // C11

    kLeftSquare,  // [
    kRightSquare, // ]
    kLeftParen,   // (
    kRightParen,  // )
    kLeftBrace,   // {
    kRightBrace,  // }
    kPeriod,      // .
    kArrow,       // ->

    kPlusPlus,   // ++
    kMinusMinus, // --

    kPostfixPlusPlus,   // ++
    kPostfixMinusMinus, // --

    kAmp,     // &
    kStar,    // *
    kPlus,    // +
    kMinus,   // -
    kTilde,   // ~
    kExclaim, // !

    kSlash,          // /
    kPercent,        // %
    kLessLess,       // <<
    kGreaterGreater, // >>
    kLess,           // <
    kGreater,        // >
    kLessEqual,      // <=
    kGreaterEqual,   // >=
    kEqualEqual,     // ==
    kExclaimEqual,   // !=
    kCaret,          // ^
    kPipe,           // |
    kAmpAmp,         // &&
    kPipePipe,       // ||

    kQuestion,  // ?
    kColon,     // :
    kSemicolon, // ;
    kEllipsis,  // ...

    kEqual,               // =
    kStarEqual,           // *=
    kSlashEqual,          // /=
    kPercentEqual,        // %=
    kPlusEqual,           // +=
    kMinusEqual,          // -=
    kLessLessEqual,       // <<=
    kGreaterGreaterEqual, // >>=
    kAmpEqual,            // &=
    kCaretEqual,          // ^=
    kPipeEqual,           // |=

    kComma,      // ,
    kSharp,      // #
    kSharpSharp, // ##

    kIdentifier,
    kInteger,
    kFloatingPoint,
    kCharacter,
    kStringLiteral,

    kOffsetof, // __builtin_offsetof
    kHugeVal,  // __builtin_huge_val
    kInff,     // __builtin_inff

    kFuncName,      // __func__ / __FUNCTION__
    kAsm,           // asm
    kAttribute,     // __attribute__
    kFuncSignature, // __PRETTY_FUNCTION__
    kExtension,     // __extension__
    kTypeof,        // typeof

    kTypeid, // typeid

    kNone,
    kEof
  };

  Q_ENUM(Values)

  static std::string ToString(TokenTag::Values value);
  static QString ToQString(TokenTag::Values value);
};

using Tag = TokenTag::Values;

class Token {
public:
  bool TagIs(Tag tag) const;
  void SetTag(Tag tag);
  Tag GetTag() const;

  std::string GetStr() const;
  void SetStr(const std::string &str);
  std::string GetIdentifier() const;

  Location GetLoc() const;
  void SetLoc(const Location &loc);

  std::string ToString() const;

  bool IsEof() const;
  bool IsIdentifier() const;
  bool IsStringLiteral() const;
  bool IsConstant() const;
  bool IsInteger() const;
  bool IsFloatPoint() const;
  bool IsCharacter() const;

  bool IsTypeSpecQual() const;
  bool IsDeclSpec() const;

private:
  Tag tag_{Tag::kNone};
  std::string str_;
  Location loc_;
};

} // namespace kcc
