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
  nlohmann::json root;
  auto str{node->KindQString()};
  root["name"] = str.append(" ").append(magic_enum::enum_name(node->GetOp()));

  nlohmann::json children;
  node->GetExpr()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const TypeCastExpr *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;
  node->GetExpr()->Accept(*this);
  children.push_back(result_);

  nlohmann::json type;
  type["name"] = "cast to: " + node->GetCastToType()->ToString();
  children.push_back(type);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const BinaryOpExpr *node) {
  nlohmann::json root;
  auto str{node->KindQString()};
  root["name"] = str.append(" ").append(magic_enum::enum_name(node->GetOp()));

  nlohmann::json children;

  node->GetLHS()->Accept(*this);
  children.push_back(result_);

  node->GetRHS()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ConditionOpExpr *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;
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
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;
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

  nlohmann::json root;
  root["name"] = str;

  result_ = root;
}

void JsonGen::Visit(const StringLiteralExpr *node) {
  nlohmann::json root;
  root["name"] = node->KindQString().append(": ").append(node->GetStr());

  result_ = root;
}

void JsonGen::Visit(const IdentifierExpr *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;

  nlohmann::json type;
  type["name"] = "type: " + node->GetType()->ToString();
  children.push_back(type);

  nlohmann::json name;
  name["name"] = "name: " + node->GetName();
  children.push_back(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const EnumeratorExpr *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;

  nlohmann::json name;
  name["name"] = "name: " + node->GetName();
  children.push_back(name);

  nlohmann::json value;
  value["name"] = node->GetVal();
  children.push_back(value);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ObjectExpr *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;

  nlohmann::json type;
  type["name"] = "type: " + node->GetType()->ToString();
  children.push_back(type);

  nlohmann::json name;
  name["name"] = "name: " + node->GetName();
  children.push_back(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const StmtExpr *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;
  node->GetBlock()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const LabelStmt *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;

  nlohmann::json name;
  name["name"] = "label: " + node->GetName();
  children.push_back(name);

  node->GetStmt()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const CaseStmt *node) {
  nlohmann::json root;

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

  nlohmann::json children;
  if (node->GetStmt()) {
    node->GetStmt()->Accept(*this);
    children.push_back(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const DefaultStmt *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;
  if (node->GetStmt()) {
    node->GetStmt()->Accept(*this);
    children.push_back(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const CompoundStmt *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;

  for (const auto &item : node->GetStmts()) {
    item->Accept(*this);
    children.push_back(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ExprStmt *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;
  if (node->GetExpr()) {
    node->GetExpr()->Accept(*this);
    children.push_back(result_);
  } else {
    nlohmann::json obj;
    obj["name"] = "empty stmt";
    children.push_back(obj);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const IfStmt *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;

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
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;
  node->GetCond()->Accept(*this);
  children.push_back(result_);

  node->GetStmt()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const WhileStmt *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;

  node->GetCond()->Accept(*this);
  children.push_back(result_);

  node->GetBlock()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const DoWhileStmt *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;

  node->GetCond()->Accept(*this);
  children.push_back(result_);

  node->GetBlock()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ForStmt *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;

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
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;
  nlohmann::json name;
  name["name"] = "label: " + node->GetName();
  children.push_back(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ContinueStmt *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();
  result_ = root;
}

void JsonGen::Visit(const BreakStmt *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();
  result_ = root;
}

void JsonGen::Visit(const ReturnStmt *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;
  if (node->GetExpr()) {
    node->GetExpr()->Accept(*this);
    children.push_back(result_);
  } else {
    nlohmann::json obj;
    obj["name"] = "void";
    children.push_back(obj);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const TranslationUnit *node) {
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;
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
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;
  node->GetIdent()->Accept(*this);
  children.push_back(result_);

  if (node->HasConstantInit()) {
    nlohmann::json obj;
    obj["name"] = LLVMConstantToStr(node->GetConstant());
    children.push_back(obj);
  } else if (node->ValueInit()) {
    nlohmann::json obj;
    obj["name"] = "value init";
    children.push_back(obj);
  } else if (node->HasLocalInit()) {
    for (const auto &item : node->GetLocalInits()) {
      nlohmann::json obj;

      std::string str;
      for (const auto &index : item.GetIndexs()) {
        str.append(std::to_string(std::get<1>(index))).append(" ");
      }

      obj["name"] = str;

      nlohmann::json arr;
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
  nlohmann::json root;
  root["name"] = node->KindQString();

  nlohmann::json children;

  node->GetIdent()->Accept(*this);
  children.push_back(result_);

  node->GetBody()->Accept(*this);
  children.push_back(result_);

  root["children"] = children;

  result_ = root;
}

}  // namespace kcc
