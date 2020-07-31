//
// Created by kaiser on 2019/10/31.
//

#include "error.h"

namespace kcc {

[[noreturn]] void Error(Tag tag, const Token &actual) {
  auto loc{actual.GetLoc()};
  fmt::print(fmt::fg(fmt::terminal_color::red), fmt("{}: error: "),
             loc.ToLocStr());
  fmt::print(fmt::fg(fmt::terminal_color::red), "expected {}, but got {}\n",
             TokenTag::ToString(tag), TokenTag::ToString(actual.GetTag()));

  fmt::print(fmt::fg(fmt::terminal_color::red), fmt("{}"),
             loc.GetLineContent());
  fmt::print(fmt::fg(fmt::terminal_color::green), fmt("{}"),
             loc.GetPositionArrow());

  PrintWarnings();
  std::exit(EXIT_FAILURE);
}

[[noreturn]] void Error(const UnaryOpExpr *unary, std::string_view msg) {
  auto loc{unary->GetLoc()};

  fmt::print(fmt::fg(fmt::terminal_color::red), fmt("{}: error: "),
             loc.ToLocStr());
  fmt::print(fmt::fg(fmt::terminal_color::red),
             "'{}': ", TokenTag::ToString(unary->GetOp()));
  fmt::print(fmt::fg(fmt::terminal_color::red), msg);
  fmt::print(fmt::fg(fmt::terminal_color::red), fmt(" (got '{}')\n"),
             unary->GetExpr()->GetQualType().ToString());

  fmt::print(fmt::fg(fmt::terminal_color::red), fmt("{}"),
             loc.GetLineContent());
  fmt::print(fmt::fg(fmt::terminal_color::green), fmt("{}"),
             loc.GetPositionArrow());

  PrintWarnings();
  std::exit(EXIT_FAILURE);
}

[[noreturn]] void Error(const BinaryOpExpr *binary, std::string_view msg) {
  auto loc{binary->GetLoc()};

  fmt::print(fmt::fg(fmt::terminal_color::red), fmt("{}: error: "),
             loc.ToLocStr());
  fmt::print(fmt::fg(fmt::terminal_color::red),
             "'{}': ", TokenTag::ToString(binary->GetOp()));
  fmt::print(fmt::fg(fmt::terminal_color::red), msg);
  fmt::print(fmt::fg(fmt::terminal_color::red), fmt(" (got '{}' and '{}')\n"),
             binary->GetLHS()->GetQualType().ToString(),
             binary->GetRHS()->GetQualType().ToString());

  fmt::print(fmt::fg(fmt::terminal_color::red), fmt("{}"),
             loc.GetLineContent());
  fmt::print(fmt::fg(fmt::terminal_color::green), fmt("{}"),
             loc.GetPositionArrow());

  PrintWarnings();
  std::exit(EXIT_FAILURE);
}

void PrintWarnings() {
  for (const auto &[item, arrow] : WarningStrings) {
    fmt::print(fmt::fg(fmt::terminal_color::white), fmt("{}"), item);
    if (!std::empty(arrow)) {
      fmt::print(fmt::fg(fmt::terminal_color::green), fmt("{}"), arrow);
    }
  }
}

} // namespace kcc
