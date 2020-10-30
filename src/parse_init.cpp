//
// Created by kaiser on 2019/12/13.
//

#include "parse.h"

#include <algorithm>
#include <cassert>

#include "calc.h"
#include "error.h"
#include "llvm_common.h"

namespace kcc {

/*
 * Init
 */
llvm::Constant *Parser::ParseInitializer(std::vector<Initializer> &inits,
                                         QualType type, bool designated,
                                         bool force_brace) {
  // 比如解析 {[2]=1}
  if (designated && !Test(Tag::kPeriod) && !Test(Tag::kLeftSquare)) {
    Expect(Tag::kEqual);
  }

  if (type->IsArrayTy()) {
    // int a[2] = 1;
    // 不能直接 Expect , 如果有 '{' , 只能由 ParseArrayInitializer 来处理
    if (force_brace && !Test(Tag::kLeftBrace) && !Test(Tag::kStringLiteral)) {
      Expect(Tag::kLeftBrace);
    } else if (auto str{ParseLiteralInitializer(type.GetType(), true)}; !str) {
      ParseArrayInitializer(inits, type.GetType(), designated);
      type->SetComplete(true);
    } else {
      return str;
    }
  } else if (type->IsStructOrUnionTy()) {
    if (!Test(Tag::kPeriod) && !Test(Tag::kLeftBrace)) {
      // struct A a = {...};
      // struct A b = a;
      // 或者是
      // struct {
      //    struct {
      //      int a;
      //      int b;
      //    } x;
      //    struct {
      //      char c[8];
      //    } y;
      //  } v = {
      //      1,
      //      2,
      //  };
      auto begin{index_};
      auto expr{ParseAssignExpr()};
      if (type->Compatible(expr->GetType())) {
        inits.emplace_back(type.GetType(), expr, indexs_);
        return nullptr;
      } else {
        index_ = begin;
      }
    }

    ParseStructInitializer(inits, type.GetType(), designated);
  } else {
    // 标量类型
    // int a={10}; / int a={10,}; 都是合法的
    auto has_brace{Try(Tag::kLeftBrace)};
    auto expr{ParseAssignExpr()};

    if (has_brace) {
      Try(Tag::kComma);
      Expect(Tag::kRightBrace);
    }

    inits.emplace_back(type.GetType(), expr, indexs_);
  }

  return nullptr;
}

void Parser::ParseArrayInitializer(std::vector<Initializer> &inits, Type *type,
                                   bool designated) {
  std::size_t index{};
  auto has_brace{Try(Tag::kLeftBrace)};

  while (true) {
    if (Test(Tag::kRightBrace)) {
      if (has_brace) {
        Next();
      }
      return;
    }

    // e.g.
    // int a[10][10] = {1, [2][2] = 3};
    if (!designated && !has_brace &&
        (Test(Tag::kPeriod) || Test(Tag::kLeftSquare))) {
      // put ',' back
      PutBack();
      return;
    }

    if ((designated = Try(Tag::kLeftSquare))) {
      auto expr{ParseAssignExpr()};
      if (!expr->GetType()->IsIntegerTy()) {
        Error(expr, "expect integer type");
      }

      index = *CalcConstantExpr{}.CalcInteger(expr);
      Expect(Tag::kRightSquare);

      if (type->IsComplete() && index >= type->ArrayGetNumElements()) {
        Error(expr, "array designator index {} exceeds array bounds", index);
      }
    }

    indexs_.push_back({type, index, 0, 0});
    ParseInitializer(inits, type->ArrayGetElementType(), designated, false);
    indexs_.pop_back();

    designated = false;
    ++index;

    if (!type->IsComplete()) {
      type->ArraySetNumElements(std::max(index, type->ArrayGetNumElements()));
    }

    if (!Try(Tag::kComma)) {
      if (has_brace) {
        Expect(Tag::kRightBrace);
      }
      return;
    }
  }
}

void Parser::ParseStructInitializer(std::vector<Initializer> &inits, Type *type,
                                    bool designated) {
  auto has_brace{Try(Tag::kLeftBrace)};
  auto member_iter{std::begin(type->StructGetMembers())};

  while (true) {
    if (Test(Tag::kRightBrace)) {
      if (has_brace) {
        Next();
      }
      return;
    }

    if ((designated = Try(Tag::kPeriod))) {
      auto tok{Expect(Tag::kIdentifier)};
      auto name{tok.GetIdentifier()};

      if (!type->StructGetMember(name)) {
        Error(tok, "member '{}' not found", name);
      }

      member_iter = GetStructDesignator(type, name);
    }

    if (member_iter == std::end(type->StructGetMembers())) {
      break;
    }

    if ((*member_iter)->IsAnonymous()) {
      if (designated) {
        PutBack();
        PutBack();
      }

      indexs_.push_back({type, (*member_iter)->GetIndexs().back().second,
                         (*member_iter)->GetBitFieldBegin(),
                         (*member_iter)->GetBitFieldWidth()});
      ParseInitializer(inits, (*member_iter)->GetType(), designated, false);
      indexs_.pop_back();
    } else {
      indexs_.push_back({type, (*member_iter)->GetIndexs().back().second,
                         (*member_iter)->GetBitFieldBegin(),
                         (*member_iter)->GetBitFieldWidth()});
      ParseInitializer(inits, (*member_iter)->GetType(), designated, false);
      indexs_.pop_back();
    }

    ++member_iter;

    if (!type->IsStructTy()) {
      break;
    }

    if (!has_brace && member_iter == std::end(type->StructGetMembers())) {
      break;
    }

    if (!Try(Tag::kComma)) {
      if (has_brace) {
        Expect(Tag::kRightBrace);
      }
      return;
    }
  }

  if (has_brace) {
    Try(Tag::kComma);
    if (!Try(Tag::kRightBrace)) {
      Error(Peek(), "excess members in struct initializer");
    }
  }
}

/*
 * ConstantInit
 */
llvm::Constant *Parser::ParseConstantInitializer(QualType type, bool designated,
                                                 bool force_brace) {
  if (designated && !Test(Tag::kPeriod) && !Test(Tag::kLeftSquare)) {
    Expect(Tag::kEqual);
  }

  if (type->IsArrayTy()) {
    if (force_brace && !Test(Tag::kLeftBrace) && !Test(Tag::kStringLiteral)) {
      Expect(Tag::kLeftBrace);
    } else if (auto p{ParseLiteralInitializer(type.GetType(), false)}; !p) {
      auto arr{ParseConstantArrayInitializer(type.GetType(), designated)};
      type->SetComplete(true);
      return arr;
    } else {
      return p;
    }
  } else if (type->IsStructOrUnionTy()) {
    return ParseConstantStructInitializer(type.GetType(), designated);
  } else {
    auto has_brace{Try(Tag::kLeftBrace)};
    auto expr{ParseAssignExpr()};

    if (has_brace) {
      Try(Tag::kComma);
      Expect(Tag::kRightBrace);
    }

    auto constant{CalcConstantExpr{}.Calc(expr)};
    if (constant) {
      return ConstantCastTo(constant, type->GetLLVMType(),
                            expr->GetType()->IsUnsigned());
    } else {
      Error(expr, "expect constant expression");
    }
  }

  assert(false);
  return nullptr;
}

llvm::Constant *Parser::ParseConstantArrayInitializer(Type *type,
                                                      bool designated) {
  std::size_t index{};
  auto has_brace{Try(Tag::kLeftBrace)};

  auto size{type->ArrayGetNumElements()};
  auto zero{GetConstantZero(type->ArrayGetElementType()->GetLLVMType())};
  std::vector<llvm::Constant *> val(size, zero);

  while (true) {
    if (Test(Tag::kRightBrace)) {
      if (has_brace) {
        Next();
      }
      return llvm::ConstantArray::get(
          llvm::cast<llvm::ArrayType>(type->GetLLVMType()), val);
    }

    if (!designated && !has_brace &&
        (Test(Tag::kPeriod) || Test(Tag::kLeftSquare))) {
      // put ',' back
      PutBack();
      return llvm::ConstantArray::get(
          llvm::cast<llvm::ArrayType>(type->GetLLVMType()), val);
    }

    if ((designated = Try(Tag::kLeftSquare))) {
      auto expr{ParseAssignExpr()};
      if (!expr->GetType()->IsIntegerTy()) {
        Error(expr, "expect integer type");
      }

      index = *CalcConstantExpr{}.CalcInteger(expr);
      Expect(Tag::kRightSquare);

      if (type->IsComplete() && index >= type->ArrayGetNumElements()) {
        Error(expr, "array designator index {} exceeds array bounds", index);
      }
    }

    if (size) {
      val[index] = ParseConstantInitializer(
          type->ArrayGetElementType().GetType(), designated, false);
    } else {
      if (index >= std::size(val)) {
        val.insert(std::end(val), index - std::size(val), zero);
        val.push_back(ParseConstantInitializer(
            type->ArrayGetElementType().GetType(), designated, false));
      } else {
        val[index] = ParseConstantInitializer(
            type->ArrayGetElementType().GetType(), designated, false);
      }
    }

    designated = false;
    ++index;

    if (type->IsComplete() && index >= type->ArrayGetNumElements()) {
      break;
    }

    if (!type->IsComplete()) {
      type->ArraySetNumElements(std::max(index, type->ArrayGetNumElements()));
    }

    if (!Try(Tag::kComma)) {
      if (has_brace) {
        Expect(Tag::kRightBrace);
      }
      return llvm::ConstantArray::get(
          llvm::cast<llvm::ArrayType>(type->GetLLVMType()), val);
    }
  }

  if (has_brace) {
    Try(Tag::kComma);
    if (!Try(Tag::kRightBrace)) {
      Error(Peek(), "excess elements in array initializer");
    }
  }

  return llvm::ConstantArray::get(
      llvm::cast<llvm::ArrayType>(type->GetLLVMType()), val);
}

llvm::Constant *Parser::ParseConstantStructInitializer(Type *type,
                                                       bool designated) {
  auto has_brace{Try(Tag::kLeftBrace)};
  auto member_iter{std::begin(type->StructGetMembers())};
  std::vector<llvm::Constant *> val;
  bool is_struct{type->IsStructTy()};

  for (std::size_t i{}; i < type->GetLLVMType()->getStructNumElements(); ++i) {
    val.push_back(
        GetConstantZero(type->GetLLVMType()->getStructElementType(i)));
  }

  while (true) {
    if (Test(Tag::kRightBrace)) {
      if (has_brace) {
        Next();
      }
      return llvm::ConstantStruct::get(
          llvm::cast<llvm::StructType>(type->GetLLVMType()), val);
    }

    if ((designated = Try(Tag::kPeriod))) {
      auto tok{Expect(Tag::kIdentifier)};
      auto name{tok.GetIdentifier()};

      if (!type->StructGetMember(name)) {
        Error(tok, "member '{}' not found", name);
      }

      member_iter = GetStructDesignator(type, name);
    }

    if (member_iter == std::end(type->StructGetMembers())) {
      break;
    }

    if ((*member_iter)->IsAnonymous()) {
      if (designated) {
        PutBack();
        PutBack();
      }
    }

    auto begin{(*member_iter)->GetBitFieldBegin()};
    auto width{(*member_iter)->GetBitFieldWidth()};
    auto index{is_struct ? (*member_iter)->GetIndexs().back().second : 0};
    auto member_type{type->GetLLVMType()->getStructElementType(index)};

    if (width) {
      llvm::Constant *old_value{
          llvm::ConstantInt::get(Builder.getInt32Ty(), 0)};

      if (member_type->isArrayTy()) {
        auto arr{val[index]};
        for (auto i{member_type->getArrayNumElements()}; i-- > 0;) {
          old_value = llvm::ConstantExpr::getOr(
              old_value,
              llvm::ConstantExpr::getShl(
                  llvm::ConstantExpr::getZExt(arr->getAggregateElement(i),
                                              Builder.getInt32Ty()),
                  llvm::ConstantInt::get(Builder.getInt32Ty(), i * 8)));
        }
      } else {
        if ((*member_iter)->GetType()->IsUnsigned()) {
          old_value =
              llvm::ConstantExpr::getZExt(val[index], Builder.getInt32Ty());
        } else {
          old_value =
              llvm::ConstantExpr::getSExt(val[index], Builder.getInt32Ty());
        }
      }

      auto size{member_type->isIntegerTy(8) ? 8 : 32};
      old_value = GetBitField(old_value, size, width, begin);

      auto new_value{ParseConstantInitializer((*member_iter)->GetType(),
                                              designated, false)};

      new_value = llvm::ConstantExpr::getShl(
          new_value, llvm::ConstantInt::get(Builder.getInt32Ty(), begin));
      new_value = llvm::ConstantExpr::getOr(old_value, new_value);

      if (member_type->isArrayTy()) {
        std::vector<llvm::Constant *> v;
        auto arr_size{member_type->getArrayNumElements()};
        for (std::size_t i{}; i < arr_size; ++i) {
          auto temp{
              llvm::ConstantExpr::getTrunc(new_value, Builder.getInt8Ty())};
          v.push_back(temp);
          new_value = llvm::ConstantExpr::getLShr(
              new_value, llvm::ConstantInt::get(Builder.getInt32Ty(), 8));
        }
        val[index] = llvm::ConstantArray::get(
            llvm::cast<llvm::ArrayType>(member_type), v);
      } else {
        val[index] =
            llvm::ConstantExpr::getTrunc(new_value, Builder.getInt8Ty());
      }
    } else {
      // 当 union 类型不对时应该新创建一个类型, 并替换
      // 但是可以正常运行, 先不管它
      val[index] = ParseConstantInitializer((*member_iter)->GetType(),
                                            designated, false);
    }

    ++member_iter;

    if (!type->IsStructTy()) {
      break;
    }

    if (!has_brace && member_iter == std::end(type->StructGetMembers())) {
      break;
    }

    if (!Try(Tag::kComma)) {
      if (has_brace) {
        Expect(Tag::kRightBrace);
      }
      return llvm::ConstantStruct::get(
          llvm::cast<llvm::StructType>(type->GetLLVMType()), val);
    }
  }

  if (has_brace) {
    Try(Tag::kComma);
    if (!Try(Tag::kRightBrace)) {
      Error(Peek(), "excess members in struct initializer");
    }
  }

  return llvm::ConstantStruct::get(
      llvm::cast<llvm::StructType>(type->GetLLVMType()), val);
}

llvm::Constant *Parser::ParseLiteralInitializer(Type *type, bool need_ptr) {
  if (!type->ArrayGetElementType()->IsIntegerTy()) {
    return nullptr;
  }

  auto has_brace{Try(Tag::kLeftBrace)};
  if (!Test(Tag::kStringLiteral)) {
    if (has_brace) {
      PutBack();
    }
    return nullptr;
  }

  auto str_node{ParseStringLiteral()};

  if (has_brace) {
    Try(Tag::kComma);
    Expect(Tag::kRightBrace);
  }

  if (!type->IsComplete()) {
    type->ArraySetNumElements(str_node->GetType()->ArrayGetNumElements());
    type->SetComplete(true);
  }

  if (str_node->GetType()->ArrayGetNumElements() >
      type->ArrayGetNumElements()) {
    Error(str_node->GetLoc(),
          "initializer-string for char array is too long '{}' to '{}",
          str_node->GetType()->ArrayGetNumElements(),
          type->ArrayGetNumElements());
  }

  if (str_node->GetType()->ArrayGetElementType()->GetWidth() !=
      type->ArrayGetElementType()->GetWidth()) {
    Error(str_node->GetLoc(), "Different character types '{}' vs '{}",
          str_node->GetType()->ArrayGetElementType()->ToString(),
          type->ArrayGetElementType()->ToString());
  }

  if (need_ptr) {
    return str_node->GetPtr();
  } else {
    return str_node->GetArr();
  }
}

}  // namespace kcc
