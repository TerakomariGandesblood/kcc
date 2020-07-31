//
// Created by kaiser on 2019/11/1.
//

#include "json_gen.h"

#include <cassert>
#include <fstream>

#include <QJsonArray>
#include <QJsonDocument>

#include "llvm_common.h"

namespace kcc {

JsonGen::JsonGen(const std::string &filter) : filter_{filter} {}

void JsonGen::GenJson(const TranslationUnit *root,
                      const std::string &file_name) {
  Visit(root);

  QJsonDocument document{result_};

  std::ofstream ofs{file_name};
  ofs << JsonGen::Before << document.toJson().toStdString() << JsonGen::After
      << std::flush;
}

bool JsonGen::CheckFileName(const AstNode *node) const {
  if (std::empty(filter_)) {
    return true;
  } else {
    return node->GetLoc().GetFileName() == filter_;
  }
}

void JsonGen::Visit(const UnaryOpExpr *node) {
  QJsonObject root;
  root["name"] = node->KindQString().append(' ').append(
      TokenTag::ToQString(node->GetOp()));

  QJsonArray children;
  node->GetExpr()->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const TypeCastExpr *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;
  node->GetExpr()->Accept(*this);
  children.append(result_);

  QJsonObject type;
  type["name"] =
      QString::fromStdString("cast to: " + node->GetCastToType()->ToString());
  children.append(type);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const BinaryOpExpr *node) {
  QJsonObject root;
  root["name"] = node->KindQString().append(' ').append(
      TokenTag::ToQString(node->GetOp()));

  QJsonArray children;

  node->GetLHS()->Accept(*this);
  children.append(result_);

  node->GetRHS()->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ConditionOpExpr *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;
  node->GetCond()->Accept(*this);
  children.append(result_);

  node->GetLHS()->Accept(*this);
  children.append(result_);

  node->GetRHS()->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const FuncCallExpr *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;
  node->GetCallee()->Accept(*this);
  children.append(result_);

  for (const auto &arg : node->GetArgs()) {
    arg->Accept(*this);
    children.append(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ConstantExpr *node) {
  auto str{node->KindQString().append(": ")};

  if (node->GetType()->IsIntegerTy()) {
    str.append(QString::number(node->GetIntegerVal().getSExtValue()));
  } else if (node->GetType()->IsFloatPointTy()) {
    str.append(QString::number(node->GetFloatPointVal().convertToDouble()));
  } else {
    assert(false);
  }

  QJsonObject root;
  root["name"] = str;

  result_ = root;
}

void JsonGen::Visit(const StringLiteralExpr *node) {
  QJsonObject root;
  root["name"] = node->KindQString().append(": ").append(
      QString::fromStdString(node->GetStr()));

  result_ = root;
}

void JsonGen::Visit(const IdentifierExpr *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;

  QJsonObject type;
  type["name"] = QString::fromStdString("type: " + node->GetType()->ToString());
  children.append(type);

  QJsonObject name;
  name["name"] = "name: " + QString::fromStdString(node->GetName());
  children.append(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const EnumeratorExpr *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;

  QJsonObject name;
  name["name"] = "name: " + QString::fromStdString(node->GetName());
  children.append(name);

  QJsonObject value;
  value["name"] = QString::number(node->GetVal());
  children.append(value);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ObjectExpr *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;

  QJsonObject type;
  type["name"] = QString::fromStdString("type: " + node->GetType()->ToString());
  children.append(type);

  QJsonObject name;
  name["name"] = "name: " + QString::fromStdString(node->GetName());
  children.append(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const StmtExpr *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;
  node->GetBlock()->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const LabelStmt *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;

  QJsonObject name;
  name["name"] = QString::fromStdString("label: " + node->GetName());
  children.append(name);

  node->GetStmt()->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const CaseStmt *node) {
  QJsonObject root;

  auto rhs{node->GetRHS()};
  if (rhs) {
    root["name"] = node->KindQString()
                       .append(' ')
                       .append(QString::number(node->GetLHS()))
                       .append("to ")
                       .append(QString::number(*rhs));
  } else {
    root["name"] =
        node->KindQString().append(' ').append(QString::number(node->GetLHS()));
  }

  QJsonArray children;
  if (node->GetStmt()) {
    node->GetStmt()->Accept(*this);
    children.append(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const DefaultStmt *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;
  if (node->GetStmt()) {
    node->GetStmt()->Accept(*this);
    children.append(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const CompoundStmt *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;

  for (const auto &item : node->GetStmts()) {
    item->Accept(*this);
    children.append(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ExprStmt *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;
  if (node->GetExpr()) {
    node->GetExpr()->Accept(*this);
    children.append(result_);
  } else {
    QJsonObject obj;
    obj["name"] = "empty stmt";
    children.append(obj);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const IfStmt *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;

  node->GetCond()->Accept(*this);
  children.append(result_);

  node->GetThen()->Accept(*this);
  children.append(result_);

  if (auto else_block{node->GetElse()}) {
    else_block->Accept(*this);
    children.append(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const SwitchStmt *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;
  node->GetCond()->Accept(*this);
  children.append(result_);

  node->GetStmt()->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const WhileStmt *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;

  node->GetCond()->Accept(*this);
  children.append(result_);

  node->GetBlock()->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const DoWhileStmt *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;

  node->GetCond()->Accept(*this);
  children.append(result_);

  node->GetBlock()->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ForStmt *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;

  if (node->GetInit()) {
    node->GetInit()->Accept(*this);
    children.append(result_);
  } else if (node->GetDecl()) {
    node->GetDecl()->Accept(*this);
    children.append(result_);
  }

  if (node->GetCond()) {
    node->GetCond()->Accept(*this);
    children.append(result_);
  }
  if (node->GetInc()) {
    node->GetInc()->Accept(*this);
    children.append(result_);
  }

  node->GetBlock()->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const GotoStmt *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;
  QJsonObject name;
  name["name"] = QString::fromStdString("label: " + node->GetName());
  children.append(name);

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const ContinueStmt *node) {
  QJsonObject root;
  root["name"] = node->KindQString();
  result_ = root;
}

void JsonGen::Visit(const BreakStmt *node) {
  QJsonObject root;
  root["name"] = node->KindQString();
  result_ = root;
}

void JsonGen::Visit(const ReturnStmt *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;
  if (node->GetExpr()) {
    node->GetExpr()->Accept(*this);
    children.append(result_);
  } else {
    QJsonObject obj;
    obj["name"] = "void";
    children.append(obj);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const TranslationUnit *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;
  for (const auto &item : node->GetExtDecl()) {
    if (!CheckFileName(item)) {
      continue;
    }
    item->Accept(*this);
    children.append(result_);
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const Declaration *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;
  node->GetIdent()->Accept(*this);
  children.append(result_);

  if (node->HasConstantInit()) {
    QJsonObject obj;
    obj["name"] =
        QString::fromStdString(LLVMConstantToStr(node->GetConstant()));
    children.append(obj);
  } else if (node->ValueInit()) {
    QJsonObject obj;
    obj["name"] = "value init";
    children.append(obj);
  } else if (node->HasLocalInit()) {
    for (const auto &item : node->GetLocalInits()) {
      QJsonObject obj;

      QString str;
      for (const auto &index : item.GetIndexs()) {
        str.append(QString::number(std::get<1>(index))).append(' ');
      }

      obj["name"] = str;

      QJsonArray arr;
      item.GetExpr()->Accept(*this);
      arr.append(result_);

      obj["children"] = arr;

      children.append(obj);
    }
  }

  root["children"] = children;

  result_ = root;
}

void JsonGen::Visit(const FuncDef *node) {
  QJsonObject root;
  root["name"] = node->KindQString();

  QJsonArray children;

  node->GetIdent()->Accept(*this);
  children.append(result_);

  node->GetBody()->Accept(*this);
  children.append(result_);

  root["children"] = children;

  result_ = root;
}

} // namespace kcc
