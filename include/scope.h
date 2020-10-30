//
// Created by kaiser on 2019/11/1.
//

#pragma once

#include <string>
#include <unordered_map>

#include "ast.h"
#include "token.h"

namespace kcc {

// C 拥有四种作用域:
// 块作用域
// 文件作用域
// 函数作用域
// 函数原型作用域
enum ScopeType { kBlock, kFile, kFunc, kFuncProto };

// 标号命名空间:所有声明为标号的标识符
// 标签名:所有声明为 struct union enum 类型名称的标识符
// 成员名:所有声明为至少为一个 struct 或 union 成员的标识符
// 每个结构体和联合体引入它自己的这种命名空间
// 所有其他标识符, 称之为通常标识符以别于(1-3)

// 在查找点, 根据使用方式确定标识符所属的命名空间
// 作为 goto 语句运算数出现的标识符, 会在标号命名空间中查找。
// 跟随关键词 struct union enum 的标识符, 会在标签命名空间中查找。
// 跟随成员访问或通过指针的成员访问运算符的标识符, 会在类型成员命名空间中查找
// 该类型由成员访问运算符左运算数确定
// 所有其他标识符, 会在通常命名空间中查找
class Scope {
 public:
  static Scope *Get(Scope *parent, enum ScopeType type);

  auto begin() { return std::begin(usual_); }
  auto end() { return std::end(usual_); }
  auto begin() const { return std::begin(usual_); }
  auto end() const { return std::end(usual_); }

  void InsertTag(IdentifierExpr *ident);
  void InsertUsual(IdentifierExpr *ident);
  void InsertTag(const std::string &name, IdentifierExpr *ident);
  void InsertUsual(const std::string &name, IdentifierExpr *ident);

  IdentifierExpr *FindTag(const std::string &name);
  IdentifierExpr *FindUsual(const std::string &name);
  IdentifierExpr *FindTagInCurrScope(const std::string &name);
  IdentifierExpr *FindUsualInCurrScope(const std::string &name);

  IdentifierExpr *FindUsual(const Token &tok);

  std::unordered_map<std::string, IdentifierExpr *> AllTagInCurrScope() const;
  Scope *GetParent();

  bool IsFileScope() const;
  bool IsBlockScope() const;

 private:
  Scope(Scope *parent, enum ScopeType type);

  Scope *parent_;
  enum ScopeType type_;

  // struct / union / enum 的名字
  std::unordered_map<std::string, IdentifierExpr *> tags_;
  // 函数 / 对象 / typedef名 / 枚举常量
  std::unordered_map<std::string, IdentifierExpr *> usual_;
};

}  // namespace kcc
