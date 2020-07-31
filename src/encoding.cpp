//
// Created by kaiser on 2019/11/8.
//

#include "encoding.h"

#include <cassert>

#include <boost/locale/encoding.hpp>
#include <boost/locale/encoding_utf.hpp>

namespace kcc {

void AppendUCN(std::string &s, std::int32_t val) {
  std::u32string str;
  str.push_back(static_cast<char32_t>(val));
  s += boost::locale::conv::utf_to_utf<char>(str);
}

void ConvertToUtf16(std::string &s) {
  s = boost::locale::conv::between(s, "UTF-16LE", "UTF-8");
}

void ConvertToUtf32(std::string &s) {
  s = boost::locale::conv::between(s, "UTF-32LE", "UTF-8");
}

void ConvertString(std::string &s, Encoding encoding) {
  switch (encoding) {
  case Encoding::kNone:
  case Encoding::kUtf8:
    break;
  case Encoding::kChar16:
    ConvertToUtf16(s);
    break;
  case Encoding::kChar32:
  case Encoding::kWchar:
    ConvertToUtf32(s);
    break;
  default:
    assert(false);
  }
}

} // namespace kcc
