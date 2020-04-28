//
// Created by kaiser on 2019/11/17.
//

#include "llvm_common.h"

#include <cassert>

#include <clang/Basic/LangOptions.h>
#include <clang/Basic/LangStandard.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/Frontend/FrontendOptions.h>
#include <llvm/ADT/Optional.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetOptions.h>

#include "error.h"

namespace kcc {

void InitLLVM() {
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  Ci.createDiagnostics();

  auto pto{std::make_shared<clang::TargetOptions>()};
  auto target_triple{llvm::sys::getDefaultTargetTriple()};
  pto->Triple = target_triple;

  TargetInfo = clang::TargetInfo::CreateTargetInfo(Ci.getDiagnostics(), pto);

  Ci.setTarget(TargetInfo);
  Ci.getInvocation().setLangDefaults(
      Ci.getLangOpts(), clang::InputKind{clang::Language::C},
      TargetInfo->getTriple(), Ci.getPreprocessorOpts(),
      clang::LangStandard::lang_c17);

  auto &lang_opt{Ci.getLangOpts()};
  lang_opt.C17 = true;
  lang_opt.Digraphs = true;
  lang_opt.Trigraphs = true;
  lang_opt.GNUMode = true;
  lang_opt.GNUKeywords = true;

  Ci.createFileManager();
  Ci.createSourceManager(Ci.getFileManager());

  Ci.createPreprocessor(clang::TranslationUnitKind::TU_Complete);

  Module = std::make_unique<llvm::Module>("", Context);
  Module->addModuleFlag(llvm::Module::Error, "wchar_size", 4);
  Module->addModuleFlag(llvm::Module::Max, "PIC Level", llvm::PICLevel::BigPIC);
  Module->addModuleFlag(llvm::Module::Max, "PIE Level", llvm::PIELevel::Large);

  std::string error;
  auto target{llvm::TargetRegistry::lookupTarget(target_triple, error)};

  if (!target) {
    Error(error);
  }

  // 使用通用CPU, 生成位置无关目标文件
  std::string cpu("generic");
  std::string features;
  llvm::TargetOptions opt;
  llvm::Optional<llvm::Reloc::Model> rm{llvm::Reloc::Model::PIC_};
  TargetMachine = std::unique_ptr<llvm::TargetMachine>{
      target->createTargetMachine(target_triple, cpu, features, opt, rm)};

  // 配置模块以指定目标机器和数据布局
  Module->setTargetTriple(target_triple);
  Module->setDataLayout(TargetMachine->createDataLayout());
}

std::string LLVMTypeToStr(llvm::Type *type) {
  assert(type != nullptr);

  std::string s;
  llvm::raw_string_ostream rso{s};
  type->print(rso);
  return s;
}

std::string LLVMConstantToStr(llvm::Constant *constant) {
  assert(constant != nullptr);

  std::string s;
  llvm::raw_string_ostream rso{s};
  constant->print(rso);
  return s;
}

llvm::Constant *GetConstantZero(llvm::Type *type) {
  assert(type != nullptr);

  if (type->isIntegerTy()) {
    return llvm::ConstantInt::get(type, 0);
  } else if (type->isFloatingPointTy()) {
    return llvm::ConstantFP::get(type, 0.0);
  } else if (type->isPointerTy()) {
    return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
  } else if (type->isAggregateType()) {
    return llvm::ConstantAggregateZero::get(type);
  } else {
    assert(false);
    return nullptr;
  }
}

std::int32_t FloatPointRank(llvm::Type *type) {
  assert(type != nullptr);

  if (type->isFloatTy()) {
    return 0;
  } else if (type->isDoubleTy()) {
    return 1;
  } else if (type->isX86_FP80Ty()) {
    return 2;
  } else {
    assert(false);
    return 0;
  }
}

bool IsArrCastToPtr(llvm::Value *value, llvm::Type *type) {
  assert(value != nullptr && type != nullptr);

  auto value_type{value->getType()};

  return value_type->isPointerTy() &&
         value_type->getPointerElementType()->isArrayTy() &&
         type->isPointerTy() &&
         value_type->getPointerElementType()
                 ->getArrayElementType()
                 ->getPointerTo() == type;
}

bool IsIntegerTy(llvm::Value *value) {
  assert(value != nullptr);
  return value->getType()->isIntegerTy();
}

bool IsFloatingPointTy(llvm::Value *value) {
  assert(value != nullptr);
  return value->getType()->isFloatingPointTy();
}

bool IsPointerTy(llvm::Value *value) {
  assert(value != nullptr);
  return value->getType()->isPointerTy();
}

bool IsFuncPointer(llvm::Type *type) {
  assert(type != nullptr);
  return type->isPointerTy() && type->getPointerElementType()->isFunctionTy();
}

bool IsArrayPointer(llvm::Type *type) {
  assert(type != nullptr);
  return type->isPointerTy() && type->getPointerElementType()->isArrayTy();
}

llvm::Constant *ConstantCastTo(llvm::Constant *value, llvm::Type *to,
                               bool is_unsigned) {
  assert(value != nullptr && to != nullptr);

  if (to->isIntegerTy(1)) {
    return ConstantCastToBool(value);
  }

  if (IsIntegerTy(value) && to->isIntegerTy()) {
    if (value->getType()->getIntegerBitWidth() > to->getIntegerBitWidth()) {
      return llvm::ConstantExpr::getTrunc(value, to);
    } else {
      if (is_unsigned) {
        return llvm::ConstantExpr::getZExt(value, to);
      } else {
        return llvm::ConstantExpr::getSExt(value, to);
      }
    }
  } else if (IsIntegerTy(value) && to->isFloatingPointTy()) {
    if (is_unsigned) {
      return llvm::ConstantExpr::getUIToFP(value, to);
    } else {
      return llvm::ConstantExpr::getSIToFP(value, to);
    }
  } else if (IsFloatingPointTy(value) && to->isIntegerTy()) {
    if (is_unsigned) {
      return llvm::ConstantExpr::getFPToUI(value, to);
    } else {
      return llvm::ConstantExpr::getFPToSI(value, to);
    }
  } else if (IsFloatingPointTy(value) && to->isFloatingPointTy()) {
    if (FloatPointRank(value->getType()) > FloatPointRank(to)) {
      return llvm::ConstantExpr::getFPTrunc(value, to);
    } else {
      return llvm::ConstantExpr::getFPExtend(value, to);
    }
  } else if (IsPointerTy(value) && to->isIntegerTy()) {
    return llvm::ConstantExpr::getPtrToInt(value, to);
  } else if (IsIntegerTy(value) && to->isPointerTy()) {
    return llvm::ConstantExpr::getIntToPtr(value, to);
  } else if (to->isVoidTy() || value->getType() == to) {
    return value;
  } else if (IsArrCastToPtr(value, to)) {
    auto zero{llvm::ConstantInt::get(Builder.getInt64Ty(), 0)};
    llvm::Constant *index[]{zero, zero};
    return llvm::ConstantExpr::getInBoundsGetElementPtr(nullptr, value, index);
  } else if (IsPointerTy(value) && to->isPointerTy()) {
    return llvm::ConstantExpr::getPointerCast(value, to);
  } else {
    Error("can not cast this expression with type '{}' to '{}'",
          LLVMTypeToStr(value->getType()), LLVMTypeToStr(to));
  }
}

llvm::Constant *ConstantCastToBool(llvm::Constant *value) {
  assert(value != nullptr);

  if (value->getType()->isIntegerTy(1)) {
    return value;
  }

  if (IsIntegerTy(value) || IsPointerTy(value)) {
    return llvm::ConstantExpr::getICmp(llvm::CmpInst::ICMP_NE, value,
                                       GetConstantZero(value->getType()));
  } else if (IsFloatingPointTy(value)) {
    return llvm::ConstantExpr::getFCmp(llvm::CmpInst::FCMP_ONE, value,
                                       GetConstantZero(value->getType()));
  } else {
    Error("this constant expression can not cast to bool: '{}'",
          LLVMTypeToStr(value->getType()));
  }
}

llvm::Value *CastTo(llvm::Value *value, llvm::Type *to, bool is_unsigned) {
  assert(value != nullptr && to != nullptr);

  if (to->isIntegerTy(1)) {
    return CastToBool(value);
  }

  if (IsIntegerTy(value) && to->isIntegerTy()) {
    if (is_unsigned) {
      return Builder.CreateZExtOrTrunc(value, to);
    } else {
      return Builder.CreateSExtOrTrunc(value, to);
    }
  } else if (IsIntegerTy(value) && to->isFloatingPointTy()) {
    if (is_unsigned) {
      return Builder.CreateUIToFP(value, to);
    } else {
      return Builder.CreateSIToFP(value, to);
    }
  } else if (IsFloatingPointTy(value) && to->isIntegerTy()) {
    if (is_unsigned) {
      return Builder.CreateFPToUI(value, to);
    } else {
      return Builder.CreateFPToSI(value, to);
    }
  } else if (IsFloatingPointTy(value) && to->isFloatingPointTy()) {
    if (FloatPointRank(value->getType()) > FloatPointRank(to)) {
      return Builder.CreateFPTrunc(value, to);
    } else {
      return Builder.CreateFPExt(value, to);
    }
  } else if (IsPointerTy(value) && to->isIntegerTy()) {
    return Builder.CreatePtrToInt(value, to);
  } else if (IsIntegerTy(value) && to->isPointerTy()) {
    return Builder.CreateIntToPtr(value, to);
  } else if (to->isVoidTy() || value->getType() == to) {
    return value;
  } else if (IsArrCastToPtr(value, to)) {
    return Builder.CreateInBoundsGEP(
        value, {Builder.getInt64(0), Builder.getInt64(0)});
  } else if (IsPointerTy(value) && to->isPointerTy()) {
    return Builder.CreatePointerCast(value, to);
  } else {
    Error("can not cast this expression with type '{}' to '{}'",
          LLVMTypeToStr(value->getType()), LLVMTypeToStr(to));
  }
}

llvm::Value *CastToBool(llvm::Value *value) {
  assert(value != nullptr);

  if (value->getType()->isIntegerTy(1)) {
    return value;
  }

  if (IsIntegerTy(value) || IsPointerTy(value)) {
    return Builder.CreateICmpNE(value, GetZero(value->getType()));
  } else if (IsFloatingPointTy(value)) {
    return Builder.CreateFCmpONE(value, GetZero(value->getType()));
  } else {
    Error("this constant expression can not cast to bool: '{}'",
          LLVMTypeToStr(value->getType()));
  }
}

llvm::Value *GetZero(llvm::Type *type) {
  assert(type != nullptr);

  if (type->isIntegerTy()) {
    return llvm::ConstantInt::get(type, 0);
  } else if (type->isFloatingPointTy()) {
    return llvm::ConstantFP::get(type, 0.0);
  } else if (type->isPointerTy()) {
    return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::GlobalVariable *CreateGlobalString(llvm::Constant *init,
                                         std::int32_t align) {
  assert(init != nullptr);
  auto var{new llvm::GlobalVariable(*Module, init->getType(), true,
                                    llvm::GlobalValue::PrivateLinkage, init,
                                    ".str")};
  var->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
  var->setAlignment(llvm::MaybeAlign{static_cast<std::uint64_t>(align)});

  return var;
}

llvm::GlobalVariable *CreateGlobalVar(const ObjectExpr *obj) {
  llvm::GlobalVariable *ptr;

  auto decl{obj->GetDecl()};
  assert(decl != nullptr);

  auto linkage{llvm::GlobalVariable::ExternalLinkage};

  if (obj->IsStatic()) {
    linkage = llvm::GlobalVariable::InternalLinkage;
  } else if (obj->IsExtern()) {
    linkage = llvm::GlobalVariable::ExternalLinkage;
  } else {
    if (!decl->HasConstantInit()) {
      linkage = llvm::GlobalVariable::CommonLinkage;
    }
  }

  auto name{obj->GetName()};

  if (auto iter{GlobalVarMap.find(name)}; iter != std::end(GlobalVarMap)) {
    ptr = iter->second;
  } else {
    ptr = new llvm::GlobalVariable(*Module, obj->GetType()->GetLLVMType(),
                                   obj->GetQualType().IsConst(), linkage,
                                   nullptr, name);
    GlobalVarMap[name] = ptr;
  }

  if (!obj->IsStatic() && !obj->IsExtern()) {
    ptr->setDSOLocal(true);
  }

  ptr->setAlignment(
      llvm::MaybeAlign{static_cast<std::uint64_t>(obj->GetType()->GetAlign())});

  if (decl->HasConstantInit()) {
    ptr->setInitializer(decl->GetConstant());
  } else {
    if (!obj->IsExtern()) {
      ptr->setInitializer(GetConstantZero(obj->GetType()->GetLLVMType()));
    }
  }

  return ptr;
}

const llvm::fltSemantics &GetFloatTypeSemantics(llvm::Type *type) {
  assert(type->isFloatingPointTy());

  if (type->isFloatTy()) {
    return TargetInfo->getFloatFormat();
  } else if (type->isDoubleTy()) {
    return TargetInfo->getDoubleFormat();
  } else if (type->isX86_FP80Ty()) {
    return TargetInfo->getLongDoubleFormat();
  } else {
    assert(false);
    return TargetInfo->getDoubleFormat();
  }
}

llvm::Type *GetBitFieldSpace(std::int8_t width) {
  if (width <= 8) {
    return Builder.getInt8Ty();
  } else {
    return llvm::ArrayType::get(Builder.getInt8Ty(), (width + 7) / 8);
  }
}

std::int32_t GetLLVMTypeSize(llvm::Type *type) {
  return Module->getDataLayout().getTypeAllocSize(type);
}

llvm::Constant *GetBitField(llvm::Constant *value, std::int32_t size,
                            std::int32_t width, std::int32_t begin) {
  if (size == 8) {
    std::uint8_t zero{};

    // ....11111
    std::uint8_t low_one;
    if (begin) {
      low_one = ~zero >> (size - begin);
    } else {
      low_one = 0;
    }

    // 111.....
    std::uint8_t high_one;
    if (auto bit{begin + width}; bit == size) {
      high_one = 0;
    } else {
      high_one = ~zero << bit;
    }

    return llvm::ConstantExpr::getAnd(
        value,
        llvm::ConstantInt::get(Builder.getInt32Ty(), low_one | high_one));
  } else if (size == 32) {
    std::uint32_t low_one;
    if (begin) {
      low_one = ~0U >> (size - begin);
    } else {
      low_one = 0;
    }

    std::uint32_t high_one;
    if (auto bit{begin + width}; bit == size) {
      high_one = 0;
    } else {
      high_one = ~0U << bit;
    }

    return llvm::ConstantExpr::getAnd(
        value,
        llvm::ConstantInt::get(Builder.getInt32Ty(), low_one | high_one));
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value *GetBitField(llvm::Value *value, std::int32_t size,
                         std::int32_t width, std::int32_t begin) {
  if (size == 8) {
    std::uint8_t zero{};

    // ....11111
    std::uint8_t low_one;
    if (begin) {
      low_one = ~zero >> (size - begin);
    } else {
      low_one = 0;
    }

    // 111.....
    std::uint8_t high_one;
    if (auto bit{begin + width}; bit == size) {
      high_one = 0;
    } else {
      high_one = ~zero << bit;
    }

    return Builder.CreateAnd(value, low_one | high_one);
  } else if (size == 32) {
    std::uint32_t low_one;
    if (begin) {
      low_one = ~0U >> (size - begin);
    } else {
      low_one = 0;
    }

    std::uint32_t high_one;
    if (auto bit{begin + width}; bit == size) {
      high_one = 0;
    } else {
      high_one = ~0U << bit;
    }

    return Builder.CreateAnd(value, low_one | high_one);
  } else {
    assert(false);
    return nullptr;
  }
}

llvm::Value *GetBitFieldValue(llvm::Value *value, std::int32_t size,
                              std::int32_t width, std::int32_t begin,
                              bool is_unsigned) {
  value = Builder.CreateShl(value, size - (begin + width));

  if (is_unsigned) {
    return Builder.CreateLShr(value, size - width);
  } else {
    return Builder.CreateAShr(value, size - width);
  }
}

}  // namespace kcc
