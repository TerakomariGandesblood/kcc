//
// Created by kaiser on 2019/11/12.
//

#include "link.h"

#include <lld/Common/Driver.h>
#include <llvm/Support/raw_ostream.h>

#include "util.h"

namespace kcc {

bool Link() {
  /*
   * Platform Specific Code
   */
  std::vector<const char *> args{
      "--eh-frame-hdr",
      "-melf_x86_64",
      "/usr/bin/../lib/gcc/x86_64-linux-gnu/10/../../../x86_64-linux-gnu/"
      "crt1.o",
      "/usr/bin/../lib/gcc/x86_64-linux-gnu/10/../../../x86_64-linux-gnu/"
      "crti.o",
      "/usr/bin/../lib/gcc/x86_64-linux-gnu/10/crtbegin.o",
      "-L/usr/bin/../lib/gcc/x86_64-linux-gnu/10",
      "-L/usr/bin/../lib/gcc/x86_64-linux-gnu/10/../../../x86_64-linux-gnu",
      "-L/usr/bin/../lib/gcc/x86_64-linux-gnu/10/../../../../lib64",
      "-L/lib/x86_64-linux-gnu",
      "-L/lib/../lib64",
      "-L/usr/lib/x86_64-linux-gnu",
      "-L/usr/lib/../lib64",
      "-L/usr/lib/x86_64-linux-gnu/../../lib64",
      "-L/usr/bin/../lib/gcc/x86_64-linux-gnu/10/../../..",
      "-L/usr/lib/llvm-11/bin/../lib",
      "-L/lib",
      "-L/usr/lib",
      "-lgcc",
      "--as-needed",
      "-lgcc_s",
      "--no-as-needed",
      "-lc",
      "-lgcc",
      "--as-needed",
      "-lgcc_s",
      "--no-as-needed",
      "/usr/bin/../lib/gcc/x86_64-linux-gnu/10/crtend.o",
      "/usr/bin/../lib/gcc/x86_64-linux-gnu/10/../../../x86_64-linux-gnu/"
      "crtn.o"};

  if (Shared) {
    args.push_back("-shared");
  } else {
    args.push_back("-dynamic-linker");
    args.push_back("/lib64/ld-linux-x86-64.so.2");
  }
  /*
   * End of Platform Specific Code
   */

  for (const auto &item : ObjFile) {
    args.push_back(item.c_str());
  }

  for (const auto &item : RPath) {
    args.push_back(item.c_str());
  }

  for (const auto &item : SoFile) {
    args.push_back(item.c_str());
  }

  for (const auto &item : AFile) {
    args.push_back(item.c_str());
  }

  for (const auto &item : Libs) {
    args.push_back(item.c_str());
  }

  std::string str{"-o" + OutputFilePath};
  args.push_back(str.c_str());

  auto level{static_cast<std::int32_t>(OptimizationLevel.getValue())};
  std::string level_str{"-plugin-opt=O" + std::to_string(level)};
  if (level != 0) {
    args.push_back("-plugin=/usr/bin/../lib/LLVMgold.so");
    args.push_back(level_str.c_str());
  }

  // TODO 后两个参数的作用
  return lld::elf::link(args, false, llvm::outs(), llvm::errs());
}

}  // namespace kcc
