//
// Created by kaiser on 2019/11/8.
//

#pragma once

#include <cstdint>
#include <string>

namespace kcc {

enum class Encoding { kNone, kUtf8, kChar16, kChar32, kWchar };

void AppendUCN(std::string &s, std::int32_t val);

void ConvertToUtf16(std::string &s);

void ConvertToUtf32(std::string &s);

void ConvertString(std::string &s, Encoding encoding);

}  // namespace kcc
