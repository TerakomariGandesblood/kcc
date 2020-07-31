//
// Created by kaiser on 2019/10/30.
//

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/Preprocessor.h>

namespace kcc {

class Preprocessor {
public:
  Preprocessor();

  void AddIncludePaths(const std::vector<std::string> &include_paths);
  // 输入形式为 name=value 或 name
  void AddMacroDefinitions(const std::vector<std::string> &macro_definitions);

  std::string Cpp(const std::string &input_file);

private:
  void AddIncludePath(const std::string &path, bool is_system);

  constexpr static std::size_t StrReserve{4096};

  clang::Preprocessor *pp_;
  clang::HeaderSearch *header_search_;
};

} // namespace kcc
