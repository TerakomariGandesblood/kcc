//
// Created by kaiser on 2019/11/8.
//

#include "encoding.h"

#include <cassert>
#include <clocale>
#include <cstddef>
#include <cuchar>
#include <stdexcept>
#include <string>

#include "error.h"

namespace kcc {

void AppendUCN(std::string &s, std::int32_t val) {
  std::string temp;

  setlocale(LC_ALL, "en_US.utf8");
  char out[6];
  char *p = out;

  mbstate_t state = {};
  size_t rc = c32rtomb(p, val, &state);
  if (rc == static_cast<std::size_t>(-1)) {
    Error("Character encoding error");
  }

  for (std::size_t i{0}; i < rc; ++i) {
    temp.push_back(out[i]);
  }

  s += temp;
}

void ConvertToUtf16(std::string &str) {
  std::setlocale(LC_ALL, "en_US.utf8");

  std::string result;

  std::size_t rc;
  char16_t out;
  auto begin = str.c_str();
  mbstate_t state = {};

  while ((rc = mbrtoc16(&out, begin, std::size(str), &state))) {
    if (rc == static_cast<std::size_t>(-3)) {
      for (std::int32_t i{0}; i < 2; ++i) {
        result.push_back(*(reinterpret_cast<char *>(&out) + i));
      }
    } else if (rc <= SIZE_MAX / 2) {
      begin += rc;

      for (std::int32_t i{0}; i < 2; ++i) {
        result.push_back(*(reinterpret_cast<char *>(&out) + i));
      }
    } else {
      break;
    }
  }

  str = result;
}

void ConvertToUtf32(std::string &str) {
  std::setlocale(LC_ALL, "en_US.utf8");

  std::string result;

  std::size_t rc;
  char32_t out;
  auto begin = str.c_str();
  mbstate_t state = {};

  while ((rc = mbrtoc32(&out, begin, std::size(str), &state))) {
    assert(rc != static_cast<std::size_t>(-3));

    if (rc > static_cast<std::size_t>(-1) / 2) {
      break;
    } else {
      begin += rc;

      for (std::int32_t i{0}; i < 4; ++i) {
        result.push_back(*(reinterpret_cast<char *>(&out) + i));
      }
    }
  }

  str = result;
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

}  // namespace kcc
