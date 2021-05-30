//
// Created by kaiser on 2019/11/1.
//

#include "json_gen.h"

#include <cassert>
#include <fstream>

#include <magic_enum.hpp>

#include "llvm_common.h"

namespace kcc {

JsonGen::JsonGen(const std::string &filter) : filter_{filter} {}

void JsonGen::GenJson(const TranslationUnit *root,
                      const std::string &file_name) {
  Visit(root);

  std::ofstream ofs{file_name};
  ofs << JsonGen::Before << result_ << JsonGen::After << std::flush;
}

bool JsonGen::CheckFileName(const AstNode *node) const {
  if (std::empty(filter_)) {
    return true;
  } else {
    return node->GetLoc().GetFileName() == filter_;
  }
}

void JsonGen::Visit(const UnaryOpExpr *node) {
  boost::json::object root;
  auto str{node->KindQString()};
  root["name"] = str.append(" ").append(magic_enum::enum_name(node->GetOp()));

  boost::json::array children;
  node->GetExpr()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const TypeCastExpr *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;
  node->GetExpr()->Accept(*this);
  children.push_back(result_);

  boost::json::object type;
  type["name"] = "cast to: " + node->GetCastToType()->ToString();
  children.push_back(type);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const BinaryOpExpr *node) {
  boost::json::object root;
  auto str{node->KindQString()};
  root["name"] = str.append(" ").append(magic_enum::enum_name(node->GetOp()));

  boost::json::array children;

  node->GetLHS()->Accept(*this);
  children.push_back(result_);

  node->GetRHS()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ConditionOpExpr *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;
  node->GetCond()->Accept(*this);
  children.push_back(result_);

  node->GetLHS()->Accept(*this);
  children.push_back(result_);

  node->GetRHS()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const FuncCallExpr *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;
  node->GetCallee()->Accept(*this);
  children.push_back(result_);

  for (const auto &arg : node->GetArgs()) {
    arg->Accept(*this);
    children.push_back(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ConstantExpr *node) {
  auto str{node->KindQString().append(": ")};

  if (node->GetType()->IsIntegerTy()) {
    str.append(std::to_string(node->GetIntegerVal().getSExtValue()));
  } else if (node->GetType()->IsFloatPointTy()) {
    str.append(std::to_string(node->GetFloatPointVal().convertToDouble()));
  } else {
    assert(false);
  }

  boost::json::object root;
  root["name"] = str;

  result_ = root;
}

void JsonGen::Visit(const StringLiteralExpr *node) {
  boost::json::object root;
  root["name"] = node->KindQString().append(": ").append(node->GetStr());

  result_ = root;
}

void JsonGen::Visit(const IdentifierExpr *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;

  boost::json::object type;
  type["name"] = "type: " + node->GetType()->ToString();
  children.push_back(type);

  boost::json::object name;
  name["name"] = "name: " + node->GetName();
  children.push_back(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const EnumeratorExpr *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;

  boost::json::object name;
  name["name"] = "name: " + node->GetName();
  children.push_back(name);

  boost::json::object value;
  value["name"] = node->GetVal();
  children.push_back(value);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ObjectExpr *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;

  boost::json::object type;
  type["name"] = "type: " + node->GetType()->ToString();
  children.push_back(type);

  boost::json::object name;
  name["name"] = "name: " + node->GetName();
  children.push_back(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const StmtExpr *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;
  node->GetBlock()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const LabelStmt *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;

  boost::json::object name;
  name["name"] = "label: " + node->GetName();
  children.push_back(name);

  node->GetStmt()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const CaseStmt *node) {
  boost::json::object root;

  auto rhs{node->GetRHS()};
  if (rhs) {
    root["name"] = node->KindQString()
                       .append(" ")
                       .append(std::to_string(node->GetLHS()))
                       .append("to ")
                       .append(std::to_string(*rhs));
  } else {
    root["name"] =
        node->KindQString().append(" ").append(std::to_string(node->GetLHS()));
  }

  boost::json::array children;
  if (node->GetStmt()) {
    node->GetStmt()->Accept(*this);
    children.push_back(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const DefaultStmt *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;
  if (node->GetStmt()) {
    node->GetStmt()->Accept(*this);
    children.push_back(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const CompoundStmt *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;

  for (const auto &item : node->GetStmts()) {
    item->Accept(*this);
    children.push_back(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ExprStmt *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;
  if (node->GetExpr()) {
    node->GetExpr()->Accept(*this);
    children.push_back(result_);
  } else {
    boost::json::object obj;
    obj["name"] = "empty stmt";
    children.push_back(obj);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const IfStmt *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;

  node->GetCond()->Accept(*this);
  children.push_back(result_);

  node->GetThen()->Accept(*this);
  children.push_back(result_);

  if (auto else_block{node->GetElse()}) {
    else_block->Accept(*this);
    children.push_back(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const SwitchStmt *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;
  node->GetCond()->Accept(*this);
  children.push_back(result_);

  node->GetStmt()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const WhileStmt *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;

  node->GetCond()->Accept(*this);
  children.push_back(result_);

  node->GetBlock()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const DoWhileStmt *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;

  node->GetCond()->Accept(*this);
  children.push_back(result_);

  node->GetBlock()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ForStmt *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;

  if (node->GetInit()) {
    node->GetInit()->Accept(*this);
    children.push_back(result_);
  } else if (node->GetDecl()) {
    node->GetDecl()->Accept(*this);
    children.push_back(result_);
  }

  if (node->GetCond()) {
    node->GetCond()->Accept(*this);
    children.push_back(result_);
  }
  if (node->GetInc()) {
    node->GetInc()->Accept(*this);
    children.push_back(result_);
  }

  node->GetBlock()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const GotoStmt *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;
  boost::json::object name;
  name["name"] = "label: " + node->GetName();
  children.push_back(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ContinueStmt *node) {
  boost::json::object root;
  root["name"] = node->KindQString();
  result_ = root;
}

void JsonGen::Visit(const BreakStmt *node) {
  boost::json::object root;
  root["name"] = node->KindQString();
  result_ = root;
}

void JsonGen::Visit(const ReturnStmt *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;
  if (node->GetExpr()) {
    node->GetExpr()->Accept(*this);
    children.push_back(result_);
  } else {
    boost::json::object obj;
    obj["name"] = "void";
    children.push_back(obj);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const TranslationUnit *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;
  for (const auto &item : node->GetExtDecl()) {
    if (!CheckFileName(item)) {
      continue;
    }
    item->Accept(*this);
    children.push_back(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const Declaration *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;
  node->GetIdent()->Accept(*this);
  children.push_back(result_);

  if (node->HasConstantInit()) {
    boost::json::object obj;
    obj["name"] = LLVMConstantToStr(node->GetConstant());
    children.push_back(obj);
  } else if (node->ValueInit()) {
    boost::json::object obj;
    obj["name"] = "value init";
    children.push_back(obj);
  } else if (node->HasLocalInit()) {
    for (const auto &item : node->GetLocalInits()) {
      boost::json::object obj;

      std::string str;
      for (const auto &index : item.GetIndexs()) {
        str.append(std::to_string(std::get<1>(index))).append(" ");
      }

      obj["name"] = str;

      boost::json::array arr;
      item.GetExpr()->Accept(*this);
      arr.push_back(result_);

      obj["children"] = arr;

      children.push_back(obj);
    }
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const FuncDef *node) {
  boost::json::object root;
  root["name"] = node->KindQString();

  boost::json::array children;

  node->GetIdent()->Accept(*this);
  children.push_back(result_);

  node->GetBody()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

}  // namespace kcc
