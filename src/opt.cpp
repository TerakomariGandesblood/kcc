//
// Created by kaiser on 2019/11/2.
//

#include "opt.h"

#include <cstdint>
#include <memory>

#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/InitializePasses.h>
#include <llvm/PassRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include "llvm_common.h"
#include "util.h"

namespace kcc {

void AddOptimizationPasses(llvm::legacy::PassManagerBase &mpm,
                           llvm::legacy::FunctionPassManager &fpm,
                           llvm::TargetMachine *tm, std::uint32_t opt_level) {
  fpm.add(llvm::createVerifierPass());

  llvm::PassManagerBuilder builder;
  builder.OptLevel = opt_level;
  builder.SizeLevel = 0;

  if (opt_level > 1) {
    builder.Inliner = llvm::createFunctionInliningPass(opt_level, 0, false);
  } else {
    builder.Inliner = llvm::createAlwaysInlinerLegacyPass();
  }

  builder.DisableUnrollLoops = false;

  if (builder.LoopVectorize) {
    builder.LoopVectorize = (opt_level > 1);
  }

  builder.SLPVectorize = (opt_level > 1);

  tm->adjustPassManager(builder);

  builder.populateFunctionPassManager(fpm);
  builder.populateModulePassManager(mpm);
}

void AddStandardLinkPasses(llvm::legacy::PassManagerBase &pm) {
  llvm::PassManagerBuilder builder;
  builder.VerifyInput = true;

  builder.Inliner = llvm::createFunctionInliningPass();
  builder.populateLTOPassManager(pm);
}

void Optimization() {
  auto level{static_cast<std::int32_t>(OptimizationLevel.getValue())};

  if (level != 0) {
    llvm::PassRegistry &registry{*llvm::PassRegistry::getPassRegistry()};

    llvm::initializeCore(registry);
    llvm::initializeCoroutines(registry);
    llvm::initializeScalarOpts(registry);
    llvm::initializeObjCARCOpts(registry);
    llvm::initializeVectorization(registry);
    llvm::initializeIPO(registry);
    llvm::initializeAnalysis(registry);
    llvm::initializeTransformUtils(registry);
    llvm::initializeInstCombine(registry);
    llvm::initializeAggressiveInstCombine(registry);
    llvm::initializeInstrumentation(registry);
    llvm::initializeTarget(registry);
    llvm::initializeExpandMemCmpPassPass(registry);
    llvm::initializeCodeGenPreparePass(registry);
    llvm::initializeAtomicExpandPass(registry);
    llvm::initializeRewriteSymbolsLegacyPassPass(registry);
    llvm::initializeWinEHPreparePass(registry);
    llvm::initializeSafeStackLegacyPassPass(registry);
    llvm::initializeSjLjEHPreparePass(registry);
    llvm::initializeStackProtectorPass(registry);
    llvm::initializePreISelIntrinsicLoweringLegacyPassPass(registry);
    llvm::initializeGlobalMergePass(registry);
    llvm::initializeIndirectBrExpandPassPass(registry);
    llvm::initializeInterleavedLoadCombinePass(registry);
    llvm::initializeInterleavedAccessPass(registry);
    llvm::initializeEntryExitInstrumenterPass(registry);
    llvm::initializePostInlineEntryExitInstrumenterPass(registry);
    llvm::initializeUnreachableBlockElimLegacyPassPass(registry);
    llvm::initializeExpandReductionsPass(registry);
    llvm::initializeWasmEHPreparePass(registry);
    llvm::initializeWriteBitcodePassPass(registry);
    llvm::initializeHardwareLoopsPass(registry);

    llvm::legacy::PassManager passes;

    llvm::TargetLibraryInfoImpl tlti(llvm::Triple{Module->getTargetTriple()});
    passes.add(new llvm::TargetLibraryInfoWrapperPass(tlti));
    passes.add(llvm::createTargetTransformInfoWrapperPass(
        TargetMachine->getTargetIRAnalysis()));

    auto fp_passes{
        std::make_unique<llvm::legacy::FunctionPassManager>(Module.get())};

    fp_passes->add(llvm::createTargetTransformInfoWrapperPass(
        TargetMachine->getTargetIRAnalysis()));

    auto &ltm{static_cast<llvm::LLVMTargetMachine &>(*TargetMachine)};
    passes.add(ltm.createPassConfig(passes));

    AddStandardLinkPasses(passes);
    AddOptimizationPasses(passes, *fp_passes, TargetMachine.get(), level);

    fp_passes->doInitialization();
    for (auto &f : *Module) {
      fp_passes->run(f);
    }
    fp_passes->doFinalization();

    passes.add(llvm::createVerifierPass());
    passes.run(*Module);
  }
}

}  // namespace kcc
