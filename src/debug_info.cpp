//
// Created by kaiser on 2019/12/10.
//

#include "debug_info.h"

#include <cassert>
#include <filesystem>

#include <llvm/ADT/SmallVector.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/IR/DebugLoc.h>
#include <llvm/IR/Module.h>

#include "scope.h"
#include "util.h"

namespace kcc {

DebugInfo::DebugInfo() {
  optimize_ = OptimizationLevel != OptLevel::kO0;

  std::filesystem::path path{Module->getSourceFileName()};
  file_ = builder_.createFile(path.filename().string(),
                              path.parent_path().string());

  cu_ = builder_.createCompileUnit(
      llvm::dwarf::DW_LANG_C11, file_, "kcc " KCC_VERSION, optimize_, "", 0, "",
      llvm::DICompileUnit::DebugEmissionKind::FullDebug, 0, true, false,
      llvm::DICompileUnit::DebugNameTableKind::None);

  Module->addModuleFlag(llvm::Module::Warning, "Dwarf Version",
                        llvm::dwarf::DWARF_VERSION);
  Module->addModuleFlag(llvm::Module::Warning, "Debug Info Version",
                        llvm::DEBUG_METADATA_VERSION);
}

void DebugInfo::Finalize() {
  assert(std::empty(lexical_blocks_));
  assert(subprogram_ == nullptr);
  assert(arg_index_ == 0);

  builder_.finalize();
}

void DebugInfo::EmitLocation(const AstNode *node) {
  if (!node) {
    return Builder.SetCurrentDebugLocation(llvm::DebugLoc{});
  }

  auto loc{node->GetLoc()};
  Builder.SetCurrentDebugLocation(llvm::DebugLoc(llvm::DILocation::get(
      Context, loc.GetRow(), loc.GetColumn(), GetScope())));
}

void DebugInfo::EmitFuncStart(const FuncDef *node) {
  assert(node != nullptr);

  auto func_type{node->GetFuncType()};
  auto func_name{func_type->FuncGetName()};

  auto line_no{node->GetLoc().GetRow()};

  subprogram_ = builder_.createFunction(file_, func_name, func_name, file_,
                                        line_no, CreateFunctionType(func_type),
                                        line_no, llvm::DINode::FlagPrototyped,
                                        llvm::DISubprogram::SPFlagDefinition);

  lexical_blocks_.push_back(subprogram_);

  auto func{Module->getFunction(func_name)};
  assert(func != nullptr);
  func->setSubprogram(subprogram_);
}

void DebugInfo::EmitFuncEnd() {
  subprogram_ = nullptr;
  arg_index_ = 0;
  lexical_blocks_.pop_back();
}

void DebugInfo::EmitParamVar(const std::string &name, Type *type,
                             llvm::AllocaInst *ptr, const Location &loc) {
  assert(subprogram_ != nullptr);

  auto line_no{loc.GetRow()};
  auto param{builder_.createParameterVariable(subprogram_, name, arg_index_++,
                                              file_, line_no,
                                              GetOrCreateType(type), true)};

  builder_.insertDeclare(
      ptr, param, builder_.createExpression(),
      llvm::DebugLoc(llvm::DILocation::get(Context, line_no, 0, subprogram_)),
      Builder.GetInsertBlock());
}

void DebugInfo::EmitLocalVar(const Declaration *decl) {
  assert(decl && decl->IsObjDecl());

  if (Builder.GetInsertBlock() == nullptr) {
    return;
  }

  llvm::DIScope *scope{lexical_blocks_.back()};
  auto ident{decl->GetIdent()};
  auto loc{decl->GetLoc()};
  auto ptr{decl->GetIdent()->ToObjectExpr()->GetLocalPtr()};

  auto var{builder_.createAutoVariable(
      scope, ident->GetName(), file_, loc.GetRow(),
      GetOrCreateType(ident->GetType()), optimize_)};

  builder_.insertDeclare(ptr, var, builder_.createExpression(),
                         llvm::DebugLoc(llvm::DILocation::get(
                             Context, loc.GetRow(), loc.GetColumn(), scope)),
                         Builder.GetInsertBlock());
}

void DebugInfo::EmitGlobalVar(const Declaration *decl) {
  assert(decl != nullptr);

  auto ptr{decl->GetIdent()->ToObjectExpr()->GetGlobalPtr()};
  auto ident{decl->GetIdent()};
  auto name{ident->GetName()};
  auto loc{decl->GetLoc()};

  auto info{builder_.createGlobalVariableExpression(
      cu_, name, name, file_, loc.GetRow(),
      GetOrCreateType(ident->GetType(), loc), ptr->hasInternalLinkage())};

  ptr->addDebugInfo(info);
}

llvm::DIScope *DebugInfo::GetScope() {
  // 该作用域既可以是 compile-unit level
  // 也可以是最近的封闭词法块(如当前函数)
  if (std::empty(lexical_blocks_)) {
    return cu_;
  } else {
    return lexical_blocks_.back();
  }
}

llvm::DIType *DebugInfo::GetOrCreateType(Type *type, const Location &loc) {
  assert(type != nullptr);

  auto &cache{type_cache_[type]};
  if (cache) {
    return cache;
  }

  if (type->IsPointerTy()) {
    cache = CreatePointerType(type);
  } else if (type->IsArrayTy()) {
    cache = CreateArrayType(type);
  } else if (type->IsStructOrUnionTy()) {
    cache = CreateStructType(type, loc);
  } else if (type->IsFunctionTy()) {
    cache = CreateFunctionType(type);
  } else {
    cache = CreateBuiltinType(type);
  }

  return cache;
}

llvm::DIType *DebugInfo::CreateBuiltinType(Type *type) {
  std::int32_t encoding{};

  if (type->IsVoidTy()) {
    return nullptr;
  } else if (type->IsCharacterTy()) {
    encoding = type->IsUnsigned() ? llvm::dwarf::DW_ATE_unsigned_char
                                  : llvm::dwarf::DW_ATE_signed_char;
  } else if (type->IsBoolTy()) {
    encoding = llvm::dwarf::DW_ATE_boolean;
  } else if (type->IsFloatPointTy()) {
    encoding = llvm::dwarf::DW_ATE_float;
  } else if (type->IsIntegerTy()) {
    encoding = type->IsUnsigned() ? llvm::dwarf::DW_ATE_unsigned
                                  : llvm::dwarf::DW_ATE_signed;
  } else {
    assert(false);
  }

  return builder_.createBasicType(type->ToString(), type->GetWidth() * 8,
                                  encoding);
}

llvm::DIType *DebugInfo::CreatePointerType(Type *type) {
  auto pointee_type{GetOrCreateType(type->PointerGetElementType().GetType())};
  auto size_in_bit{type->GetWidth() * 8};
  auto align_in_bit{type->GetAlign() * 8};

  return builder_.createPointerType(pointee_type, size_in_bit, align_in_bit);
}

llvm::DIType *DebugInfo::CreateArrayType(Type *type) {
  std::int32_t size{}, align{};

  if (type->IsComplete()) {
    size = type->GetWidth();
    align = type->GetAlign();
  } else {
    align = type->ArrayGetElementType()->GetAlign();
  }

  // 下标范围
  llvm::SmallVector<llvm::Metadata *, 8> subscripts;
  QualType qual_type{type};
  while (qual_type->IsArrayTy()) {
    subscripts.push_back(
        builder_.getOrCreateSubrange(0, type->ArrayGetNumElements()));
    qual_type = qual_type->ArrayGetElementType();
  }

  return builder_.createArrayType(
      size, align * 8, GetOrCreateType(type->ArrayGetElementType().GetType()),
      builder_.getOrCreateArray(subscripts));
}

llvm::DIType *DebugInfo::CreateStructType(Type *type, const Location &loc) {
  std::int32_t tag{};
  if (type->IsStructTy()) {
    tag = llvm::dwarf::DW_TAG_structure_type;
  } else if (type->IsUnionTy()) {
    tag = llvm::dwarf::DW_TAG_union_type;
  } else {
    assert(false);
  }

  std::string name{type->StructGetName()};
  auto scope{GetScope()};

  auto fwd_type{
      builder_.createForwardDecl(tag, name, scope, file_, loc.GetRow())};
  if (!type->IsComplete()) {
    return fwd_type;
  }

  type_cache_[type] = fwd_type;

  llvm::SmallVector<llvm::Metadata *, 16> ele_types;
  for (const auto &[name, ident] : *type->StructGetScope()) {
    auto member_type{GetOrCreateType(ident->GetType(), ident->GetLoc())};

    if (std::empty(name)) {
      continue;
    }

    auto line{ident->GetLoc().GetRow()};
    std::int32_t size_in_bit{};
    std::int32_t align_in_bit{};
    std::int32_t offset_in_bit{ident->ToObjectExpr()->GetOffset() * 8};

    if (!(ident->GetType()->IsArrayTy() && !ident->GetType()->IsComplete())) {
      size_in_bit = ident->GetType()->GetWidth() * 8;
      align_in_bit = ident->GetType()->GetAlign() * 8;
    }

    member_type = builder_.createMemberType(
        scope, name, file_, line, size_in_bit, align_in_bit, offset_in_bit,
        llvm::DINode::DIFlags::FlagPublic, member_type);

    ele_types.push_back(member_type);
  }

  auto real_type{builder_.createStructType(
      scope, name, file_, loc.GetRow(), type->GetWidth() * 8,
      type->GetAlign() * 8, llvm::DINode::DIFlags::FlagZero, nullptr,
      builder_.getOrCreateArray(ele_types))};

  type_cache_[type] = real_type;

  return real_type;
}

llvm::DISubroutineType *DebugInfo::CreateFunctionType(Type *type) {
  llvm::SmallVector<llvm::Metadata *, 8> ele_types;

  ele_types.push_back(GetOrCreateType(type->FuncGetReturnType().GetType()));

  for (const auto &item : type->FuncGetParams()) {
    ele_types.push_back(GetOrCreateType(item->GetType()));
  }

  return builder_.createSubroutineType(
      builder_.getOrCreateTypeArray(ele_types));
}

}  // namespace kcc
