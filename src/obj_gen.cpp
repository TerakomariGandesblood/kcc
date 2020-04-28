//
// Created by kaiser on 2019/11/2.
//

#include "obj_gen.h"

#include <system_error>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include "error.h"
#include "llvm_common.h"

namespace kcc {

void ObjGen(const std::string& obj_file, llvm::CodeGenFileType file_type) {
  std::error_code error_code;
  llvm::raw_fd_ostream dest{obj_file, error_code, llvm::sys::fs::F_None};

  if (error_code) {
    Error("Could not open file: '{}'", error_code.message());
  }

  // 定义 PassManager 以生成目标代码
  llvm::legacy::PassManager pass;

  if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
    Error("The TargetMachine can't emit a file of this type");
  }

  pass.run(*Module);
  dest.flush();
}

}  // namespace kcc
