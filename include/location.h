//
// Created by kaiser on 2019/11/2.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace kcc {

class Location {
 public:
  void SetFileName(const std::string &file_name);
  void SetContent(const char *content);
  void NextRow(std::size_t line_begin);
  void NextColumn();
  void PrevRow();
  void PrevColumn();
  void SetRow(std::int32_t row);
  std::string GetFileName() const;

  std::string ToLocStr() const;
  std::string GetLineContent() const;
  std::string GetPositionArrow() const;

  std::int32_t GetRow() const;
  std::int32_t GetColumn() const;

 private:
  std::string file_name_;
  const char *content_;

  std::size_t line_begin_{};
  std::int32_t row_{1};
  std::int32_t column_{1};

  std::size_t line_begin_backup_{};
  std::int32_t column_backup_{};
};

}  // namespace kcc
