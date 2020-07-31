//
// Created by kaiser on 2019/12/10.
//

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Instructions.h>

#include "ast.h"
#include "location.h"
#include "type.h"

namespace kcc {

// LLVM 中使用的是 DWARF
// 它是一种广泛使用的标准化调试数据格式
class DebugInfo {
public:
  DebugInfo();
  // 代码生成完毕之后调用它
  void Finalize();

  void EmitLocation(const AstNode *node);

  void EmitFuncStart(const FuncDef *node);
  void EmitFuncEnd();

  void EmitParamVar(const std::string &name, Type *type, llvm::AllocaInst *ptr,
                    const Location &loc);
  void EmitLocalVar(const Declaration *node);
  void EmitGlobalVar(const Declaration *node);

private:
  llvm::DIScope *GetScope();

  llvm::DIType *GetOrCreateType(Type *type, const Location &loc = Location{});

  llvm::DIType *CreateBuiltinType(Type *type);
  llvm::DIType *CreatePointerType(Type *type);
  llvm::DIType *CreateArrayType(Type *type);
  llvm::DIType *CreateStructType(Type *type, const Location &loc);
  llvm::DISubroutineType *CreateFunctionType(Type *type);

  bool optimize_;
  // DWARF 中一段代码的顶级容器, 它包含单个翻译单元类型和函数数据
  llvm::DICompileUnit *cu_;
  llvm::DIFile *file_;
  // 类似与 IRBuilder
  llvm::DIBuilder *builder_;
  std::vector<llvm::DIScope *> lexical_blocks_;

  llvm::DISubprogram *subprogram_{};
  std::int32_t arg_index_{};

  std::unordered_map<Type *, llvm::DIType *> type_cache_;
};

} // namespace kcc
