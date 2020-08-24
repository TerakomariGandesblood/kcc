//
// Created by kaiser on 2019/10/30.
//

#pragma once

#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/color.h>
#include <fmt/format.h>

#include "ast.h"
#include "location.h"
#include "token.h"

namespace kcc {

inline std::vector<std::pair<std::string, std::string>> WarningStrings;

[[noreturn]] void Error(Tag expect, const Token &actual);
[[noreturn]] void Error(const UnaryOpExpr *unary, std::string_view msg);
[[noreturn]] void Error(const BinaryOpExpr *binary, std::string_view msg);
void PrintWarnings();

template <typename... Args>
[[noreturn]] void Error(std::string_view format_str, const Args &... args) {
  fmt::print(fmt::fg(fmt::terminal_color::red), "error: ");
  fmt::print(fmt::fg(fmt::terminal_color::red), format_str, args...);
  fmt::print("\n");

  PrintWarnings();
  std::exit(EXIT_FAILURE);
}

template <typename... Args>
[[noreturn]] void Error(const Location &loc, std::string_view format_str,
                        const Args &... args) {
  fmt::print(fmt::fg(fmt::terminal_color::red), FMT_STRING("{}: error: "),
             loc.ToLocStr());
  fmt::print(fmt::fg(fmt::terminal_color::red), format_str, args...);
  fmt::print("\n");

  fmt::print(fmt::fg(fmt::terminal_color::red), FMT_STRING("{}"),
             loc.GetLineContent());
  fmt::print(fmt::fg(fmt::terminal_color::green), FMT_STRING("{}"),
             loc.GetPositionArrow());

  PrintWarnings();
  std::exit(EXIT_FAILURE);
}

template <typename... Args>
[[noreturn]] void Error(const Token &tok, std::string_view format_str,
                        const Args &... args) {
  Error(tok.GetLoc(), format_str, args...);
}

template <typename... Args>
[[noreturn]] void Error(const Expr *expr, std::string_view format_str,
                        const Args &... args) {
  Error(expr->GetLoc(), format_str, args...);
}

template <typename... Args>
void Warning(std::string_view format_str, const Args &... args) {
  std::string str{"warning: "};

  str += fmt::format(format_str, args...);
  str += '\n';

  WarningStrings.push_back({str, ""});
}

template <typename... Args>
void Warning(const Location &loc, std::string_view format_str,
             const Args &... args) {
  std::string str;

  str += fmt::format(FMT_STRING("{}: warning: "), loc.ToLocStr());
  str += fmt::format(format_str, args...);
  str += '\n';
  str += loc.GetLineContent();

  WarningStrings.push_back({str, loc.GetPositionArrow()});
}

template <typename... Args>
void Warning(const Token &tok, std::string_view format_str,
             const Args &... args) {
  Warning(tok.GetLoc(), format_str, args...);
}

} // namespace kcc
