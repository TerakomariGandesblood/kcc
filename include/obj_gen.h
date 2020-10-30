//
// Created by kaiser on 2019/11/2.
//

#pragma once

#include <string>

#include <llvm/Target/TargetMachine.h>

namespace kcc {

void ObjGen(
    const std::string &obj_file,
    llvm::CodeGenFileType file_type = llvm::CodeGenFileType::CGFT_ObjectFile);

}  // namespace kcc
