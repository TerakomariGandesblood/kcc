//
// Created by kaiser on 2019/11/2.
//

#include "location.h"

#include <cassert>
#include <cstring>

#include <fmt/format.h>

namespace kcc {

void Location::SetFileName(const std::string &file_name) {
  assert(!std::empty(file_name));
  file_name_ = file_name;
}

void Location::SetContent(const char *content) {
  assert(content != nullptr);
  content_ = content;
}

void Location::NextColumn() { ++column_; }

void Location::NextRow(std::size_t line_begin) {
  line_begin_backup_ = line_begin;
  column_backup_ = column_;

  line_begin_ = line_begin;
  ++row_;
  column_ = 1;
}

void Location::PrevRow() {
  assert(row_ > 1);

  line_begin_ = line_begin_backup_;
  column_ = column_backup_;

  --row_;
}

void Location::PrevColumn() {
  assert(column_ > 1);
  --column_;
}

// row 代表的是下一行的行号
void Location::SetRow(std::int32_t row) {
  assert(row >= 0);
  row_ = row;
}

std::string Location::GetFileName() const {
  assert(!std::empty(file_name_));
  return file_name_;
}

std::string Location::ToLocStr() const {
  assert(!std::empty(file_name_));
  return fmt::format(FMT_STRING("{}:{}:{}"), file_name_, row_, column_);
}

std::string Location::GetLineContent() const {
  assert(content_ != nullptr);

  std::string str;
  auto size{std::strlen(content_)};

  for (auto index{line_begin_}; index < size && content_[index] != '\n';
       ++index) {
    str.push_back(content_[index]);
  }

  str += '\n';

  return str;
}

std::string Location::GetPositionArrow() const {
  return fmt::format(FMT_STRING("{}{}\n"), std::string(column_ - 1, ' '), "^");
}

std::int32_t Location::GetRow() const { return row_; }

std::int32_t Location::GetColumn() const { return column_; }

} // namespace kcc
