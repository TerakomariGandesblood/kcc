//
// Created by kaiser on 2019/10/31.
//

#include "dict.h"

namespace kcc {

KeywordsDictionary::KeywordsDictionary() {
  keywords_.insert({"auto", Tag::kAuto});
  keywords_.insert({"break", Tag::kBreak});
  keywords_.insert({"case", Tag::kCase});
  keywords_.insert({"char", Tag::kChar});
  keywords_.insert({"const", Tag::kConst});
  keywords_.insert({"continue", Tag::kContinue});
  keywords_.insert({"default", Tag::kDefault});
  keywords_.insert({"do", Tag::kDo});
  keywords_.insert({"double", Tag::kDouble});
  keywords_.insert({"else", Tag::kElse});
  keywords_.insert({"enum", Tag::kEnum});
  keywords_.insert({"extern", Tag::kExtern});
  keywords_.insert({"float", Tag::kFloat});
  keywords_.insert({"for", Tag::kFor});
  keywords_.insert({"goto", Tag::kGoto});
  keywords_.insert({"if", Tag::kIf});
  keywords_.insert({"inline", Tag::kInline});
  keywords_.insert({"int", Tag::kInt});
  keywords_.insert({"long", Tag::kLong});
  keywords_.insert({"register", Tag::kRegister});
  keywords_.insert({"restrict", Tag::kRestrict});
  keywords_.insert({"return", Tag::kReturn});
  keywords_.insert({"short", Tag::kShort});
  keywords_.insert({"signed", Tag::kSigned});
  keywords_.insert({"sizeof", Tag::kSizeof});
  keywords_.insert({"static", Tag::kStatic});
  keywords_.insert({"struct", Tag::kStruct});
  keywords_.insert({"switch", Tag::kSwitch});
  keywords_.insert({"typedef", Tag::kTypedef});
  keywords_.insert({"union", Tag::kUnion});
  keywords_.insert({"unsigned", Tag::kUnsigned});
  keywords_.insert({"void", Tag::kVoid});
  keywords_.insert({"volatile", Tag::kVolatile});
  keywords_.insert({"while", Tag::kWhile});
  keywords_.insert({"_Alignas", Tag::kAlignas});
  keywords_.insert({"_Alignof", Tag::kAlignof});
  keywords_.insert({"_Atomic", Tag::kAtomic});
  keywords_.insert({"_Bool", Tag::kBool});
  keywords_.insert({"_Complex", Tag::kComplex});
  keywords_.insert({"_Generic", Tag::kGeneric});
  keywords_.insert({"_Imaginary", Tag::kImaginary});
  keywords_.insert({"_Noreturn", Tag::kNoreturn});
  keywords_.insert({"_Static_assert", Tag::kStaticAssert});
  keywords_.insert({"_Thread_local", Tag::kThreadLocal});

  keywords_.insert({"__func__", Tag::kFuncName});
  keywords_.insert({"__builtin_offsetof", Tag::kOffsetof});
  keywords_.insert({"__builtin_huge_val", Tag::kHugeVal});
  keywords_.insert({"__builtin_inff", Tag::kInff});

  // GNU 扩展
  keywords_.insert({"typeof", Tag::kTypeof});
  keywords_.insert({"__typeof__", Tag::kTypeof});
  keywords_.insert({"__attribute__", Tag::kAttribute});
  keywords_.insert({"__extension__", Tag::kExtension});
  keywords_.insert({"__FUNCTION__", Tag::kFuncName});
  keywords_.insert({"__PRETTY_FUNCTION__", Tag::kFuncSignature});
  keywords_.insert({"_Float32", Tag::kFloat});
  keywords_.insert({"_Float64", Tag::kDouble});
  keywords_.insert({"_Float32x", Tag::kDouble});
  // FIXME
  keywords_.insert({"_Float64x", Tag::kDouble});
  keywords_.insert({"_Float128", Tag::kDouble});

  keywords_.insert({"__inline", Tag::kInline});
  keywords_.insert({"__alignof__", Tag::kAlignof});
  keywords_.insert({"__inline__", Tag::kInline});
  keywords_.insert({"__restrict", Tag::kRestrict});
  keywords_.insert({"__restrict__", Tag::kRestrict});
  keywords_.insert({"__signed__", Tag::kSigned});
  keywords_.insert({"__volatile__", Tag::kVolatile});
  keywords_.insert({"asm", Tag::kAsm});
  keywords_.insert({"__asm__", Tag::kAsm});
  keywords_.insert({"__asm", Tag::kAsm});

  // 一个显示表达式类型名称的扩展
  keywords_.insert({"typeid", Tag::kTypeid});
}

Tag KeywordsDictionary::Find(const std::string& name) const {
  if (auto iter{keywords_.find(name)}; iter != std::end(keywords_)) {
    return iter->second;
  } else {
    return Tag::kIdentifier;
  }
}

}  // namespace kcc
