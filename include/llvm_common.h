//
// Created by kaiser on 2019/11/17.
//

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <clang/Basic/TargetInfo.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/Target/TargetMachine.h>

#include "ast.h"

namespace kcc {

// 拥有许多 LLVM 核心数据结构, 如类型和常量值表
inline llvm::LLVMContext Context;
// 一个辅助对象, 跟踪当前位置并且可以插入 LLVM 指令
inline llvm::IRBuilder<> Builder{Context};
// 包含函数和全局变量, 它拥有生成的所有 IR 的内存
inline std::unique_ptr<llvm::Module> Module;

inline clang::TargetInfo *TargetInfo;

inline std::unique_ptr<llvm::TargetMachine> TargetMachine;

inline clang::CompilerInstance Ci;

void InitLLVM();

std::string LLVMTypeToStr(llvm::Type *type);

std::string LLVMConstantToStr(llvm::Constant *constant);

llvm::Constant *GetConstantZero(llvm::Type *type);

std::int32_t FloatPointRank(llvm::Type *type);

bool IsArrCastToPtr(llvm::Value *value, llvm::Type *type);

bool IsIntegerTy(llvm::Value *value);

bool IsFloatingPointTy(llvm::Value *value);

bool IsPointerTy(llvm::Value *value);

bool IsFuncPointer(llvm::Type *type);

bool IsArrayPointer(llvm::Type *type);

llvm::Constant *ConstantCastToBool(llvm::Constant *value);

llvm::Constant *ConstantCastTo(llvm::Constant *value, llvm::Type *to,
                               bool is_unsigned);

llvm::Value *CastTo(llvm::Value *value, llvm::Type *to, bool is_unsigned);

llvm::Value *CastToBool(llvm::Value *value);

llvm::Value *GetZero(llvm::Type *type);

llvm::GlobalVariable *CreateGlobalString(llvm::Constant *init,
                                         std::int32_t align);

llvm::GlobalVariable *CreateGlobalVar(const ObjectExpr *obj);

const llvm::fltSemantics &GetFloatTypeSemantics(llvm::Type *type);

llvm::Type *GetBitFieldSpace(std::int8_t width);

std::int32_t GetLLVMTypeSize(llvm::Type *type);

llvm::Constant *GetBitField(llvm::Constant *value, std::int32_t size,
                            std::int32_t width, std::int32_t begin);

llvm::Value *GetBitField(llvm::Value *value, std::int32_t size,
                         std::int32_t width, std::int32_t begin);

llvm::Value *GetBitFieldValue(llvm::Value *value, std::int32_t size,
                              std::int32_t width, std::int32_t begin,
                              bool is_unsigned);

} // namespace kcc
