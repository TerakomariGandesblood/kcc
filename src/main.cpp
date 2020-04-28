//
// Created by kaiser on 2019/10/30.
//

#include <unistd.h>
#include <wait.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

#include "code_gen.h"
#include "cpp.h"
#include "error.h"
#include "json_gen.h"
#include "lex.h"
#include "link.h"
#include "llvm_common.h"
#include "obj_gen.h"
#include "opt.h"
#include "parse.h"
#include "util.h"

using namespace kcc;

void RunKcc(const std::string &file_name);

#ifdef DEV
#include <cstdlib>
void RunDev();
#endif

int main(int argc, char *argv[]) try {
  InitLLVM();

  InitCommandLine(argc, argv);
  CommandLineCheck();

#ifdef DEV
  if (DevMode) {
    RunDev();
    return EXIT_SUCCESS;
  }
#endif

  TimingStart();
  for (const auto &item : InputFilePaths) {
    auto pid{fork()};
    if (pid < 0) {
      Error("fork error");
    } else if (pid == 0) {
      RunKcc(item);
      PrintWarnings();
      return EXIT_SUCCESS;
    }
  }

  std::int32_t status{};

  while (waitpid(-1, &status, 0) > 0) {
    // WIFEXITED 如果通过调用 exit 或者 return 正常终止, 则为真
    // WEXITSTATUS 返回一个正常终止的子进程的退出状态, 只有在
    // WIFEXITED 为真时才会定义
    if (!WIFEXITED(status) || WEXITSTATUS(status)) {
      Error("Compile Error");
    }
  }

  if (DoNotLink()) {
    TimingEnd("Timing");
    return EXIT_SUCCESS;
  }

  if (std::empty(OutputFilePath)) {
    OutputFilePath = "a.out";
  }

  if (!Link()) {
    RemoveFiles();
    Error("Link Failed");
  } else {
    RemoveFiles();
  }

  TimingEnd("Timing");
} catch (const std::exception &error) {
  Error("{}", error.what());
}

void RunKcc(const std::string &file_name) {
  Preprocessor preprocessor;
  preprocessor.AddIncludePaths(IncludePaths);
  preprocessor.AddMacroDefinitions(MacroDefines);

  auto preprocessed_code{preprocessor.Cpp(file_name)};

  if (Preprocess) {
    if (std::empty(OutputFilePath)) {
      std::cout << preprocessed_code << '\n' << std::endl;
    } else {
      std::ofstream ofs{OutputFilePath};
      ofs << preprocessed_code << std::endl;
    }
    return;
  }

  Scanner scanner{std::move(preprocessed_code)};
  auto tokens{scanner.Tokenize()};

  if (EmitTokens) {
    if (std::empty(OutputFilePath)) {
      for (const auto &tok : tokens) {
        if (tok.GetLoc().GetFileName() == file_name) {
          std::cout << tok.ToString() << '\n';
        }
      }
      std::cout << std::endl;
    } else {
      std::ofstream ofs{OutputFilePath};
      for (const auto &tok : tokens) {
        if (tok.GetLoc().GetFileName() == file_name) {
          ofs << tok.ToString() << '\n';
        }
      }
      ofs << std::endl;
    }
    return;
  }

  Parser parser{std::move(tokens)};
  auto unit{parser.ParseTranslationUnit()};

  if (EmitAST) {
    JsonGen json_gen{file_name};
    if (std::empty(OutputFilePath)) {
      json_gen.GenJson(unit, GetFileName(file_name, ".html"));
    } else {
      json_gen.GenJson(unit, OutputFilePath);
    }
    return;
  }

  CodeGen code_gen;
  code_gen.GenCode(unit);
  Optimization();

  if (EmitLLVM) {
    std::error_code error_code;
    if (std::empty(OutputFilePath)) {
      llvm::raw_fd_ostream ir_file{GetFileName(file_name, ".ll"), error_code};
      ir_file << *Module;
    } else {
      llvm::raw_fd_ostream ir_file{OutputFilePath, error_code};
      ir_file << *Module;
    }
    return;
  }

  if (OutputAssembly) {
    if (std::empty(OutputFilePath)) {
      ObjGen(GetFileName(file_name, ".s"),
             llvm::CodeGenFileType::CGFT_AssemblyFile);
    } else {
      ObjGen(OutputFilePath, llvm::CodeGenFileType::CGFT_AssemblyFile);
    }
    return;
  }

  if (OutputObjectFile) {
    if (std::empty(OutputFilePath)) {
      ObjGen(GetFileName(file_name, ".o"));
    } else {
      ObjGen(OutputFilePath);
    }
    return;
  }

  ObjGen(GetObjFile(file_name));
}

#ifdef DEV
void Run(const std::string &file) {
  Preprocessor preprocessor;
  preprocessor.AddIncludePaths(IncludePaths);
  preprocessor.AddMacroDefinitions(MacroDefines);

  auto preprocessed_code{preprocessor.Cpp(file)};
  std::ofstream preprocess_file{GetFileName(file, ".i")};
  preprocess_file << preprocessed_code << std::flush;

  Scanner scanner{std::move(preprocessed_code)};
  auto tokens{scanner.Tokenize()};
  std::ofstream tokens_file{GetFileName(file, ".txt")};
  for (const auto &tok : tokens) {
    if (tok.GetLoc().GetFileName() == file) {
      tokens_file << tok.ToString() << '\n';
    }
  }
  tokens_file << std::flush;

  Parser parser{std::move(tokens)};
  auto unit{parser.ParseTranslationUnit()};
  JsonGen{file}.GenJson(unit, GetFileName(file, ".html"));

  std::string cmd{"clang -std=gnu17 -S -emit-llvm -O0 -g " + file + " -o " +
                  GetFileName(file, ".std.ll")};
  std::system(cmd.c_str());

  cmd = "./clang -std=gnu99 -S -emit-llvm -O0 -g " + file + " -o " +
        GetFileName(file, ".old.ll");
  std::system(cmd.c_str());

  CodeGen code_gen;
  code_gen.GenCode(unit);

  Optimization();

  std::error_code error_code;
  llvm::raw_fd_ostream ir_file{GetFileName(file, ".ll"), error_code};
  ir_file << *Module;

  cmd = "llc " + GetFileName(file, ".ll");
  if (!CommandSuccess(std::system(cmd.c_str()))) {
    Error("emit asm fail");
  }

  ObjGen(GetObjFile(file));
}

void RunDev() {
  assert(std::size(InputFilePaths) == 1);
  auto file{InputFilePaths.front()};
  Run(file);

  std::cout << "link\n";
  OutputFilePath = GetFileName(file, ".out");
  if (!Link()) {
    Error("link fail");
  }
  EnsureFileExists(GetFileName(file, ".out"));

  std::string cmd{"./" + GetFileName(file, ".out")};
  if (!CommandSuccess(std::system(cmd.c_str()))) {
    Error("run fail");
  }

  std::cout << "-----------------------------\n";

  std::cout << "lli\n";
  cmd = "lli " + GetFileName(file, ".ll");
  if (!CommandSuccess(std::system(cmd.c_str()))) {
    Error("run fail");
  }

  PrintWarnings();
}
#endif
