//
// Created by kaiser on 2019/10/30.
//

#include <unistd.h>
#include <wait.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
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
#include "obj_gen.h"
#include "opt.h"
#include "parse.h"
#include "util.h"

using namespace kcc;

void RunKcc(const std::string &file_name);
#ifdef DEV
void RunTest();
void RunDev();
#endif

int main(int argc, char *argv[]) try {
  TimingStart();

  InitCommandLine(argc, argv);
  CommandLineCheck();

#ifdef DEV
  if (TestMode) {
    RunTest();
    return EXIT_SUCCESS;
  } else if (DevMode) {
    RunDev();
    return EXIT_SUCCESS;
  }
#endif

  // FIXME 编译多文件时行为不正确
  for (const auto &item : InputFilePaths) {
    auto pid{fork()};
    if (pid < 0) {
      Error("fork error");
    } else if (pid == 0) {
      RunKcc(item);
      PrintWarnings();
      std::exit(EXIT_SUCCESS);
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

  if (Preprocess || OutputAssembly || OutputObjectFile || EmitTokens ||
      EmitAST || EmitLLVM) {
    if (Timing) {
      TimingEnd("Timing:");
    }
    return EXIT_SUCCESS;
  }

  if (std::empty(OutputFilePath)) {
    OutputFilePath = "a.out";
  }

  auto obj_files{GetObjFiles()};
  if (!Link(obj_files, OptimizationLevel, OutputFilePath)) {
    Error("Link Failed");
  } else {
    RemoveAllFiles(obj_files);
  }

  if (Timing) {
    TimingEnd("Timing:");
  }
} catch (const std::exception &error) {
  Error("{}", error.what());
}

void RunKcc(const std::string &file_name) {
  Preprocessor preprocessor;
  preprocessor.SetIncludePaths(IncludePaths);
  preprocessor.SetMacroDefinitions(MacroDefines);

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
      std::transform(std::begin(tokens), std::end(tokens),
                     std::ostream_iterator<std::string>{std::cout, "\n"},
                     std::mem_fn(&Token::ToString));
      std::cout << std::endl;
    } else {
      std::ofstream ofs{OutputFilePath};
      std::transform(std::begin(tokens), std::end(tokens),
                     std::ostream_iterator<std::string>{ofs, "\n"},
                     std::mem_fn(&Token::ToString));
      ofs << std::flush;
    }
    return;
  }

  Parser parser{std::move(tokens)};
  auto unit{parser.ParseTranslationUnit()};

  if (EmitAST) {
    JsonGen json_gen;
    if (std::empty(OutputFilePath)) {
      json_gen.GenJson(unit, GetFileName(file_name, ".html"));
    } else {
      json_gen.GenJson(unit, OutputFilePath);
    }
    return;
  }

  CodeGen code_gen{file_name};
  code_gen.GenCode(unit);
  Optimization(OptimizationLevel);

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
             llvm::TargetMachine::CodeGenFileType::CGFT_AssemblyFile);
    } else {
      ObjGen(OutputFilePath,
             llvm::TargetMachine::CodeGenFileType::CGFT_AssemblyFile);
    }
    return;
  }

  if (OutputObjectFile) {
    if (std::empty(OutputFilePath)) {
      ObjGen(GetFileName(file_name, ".o"),
             llvm::TargetMachine::CodeGenFileType::CGFT_ObjectFile);
    } else {
      ObjGen(OutputFilePath,
             llvm::TargetMachine::CodeGenFileType::CGFT_ObjectFile);
    }
    return;
  }

  ObjGen(GetObjFile(file_name),
         llvm::TargetMachine::CodeGenFileType::CGFT_ObjectFile);
}

#ifdef DEV
void Run(const std::string &file) {
  Preprocessor preprocessor;
  preprocessor.SetIncludePaths(IncludePaths);
  preprocessor.SetMacroDefinitions(MacroDefines);

  auto preprocessed_code{preprocessor.Cpp(file)};
  std::ofstream preprocess_file{GetFileName(file, ".i")};
  preprocess_file << preprocessed_code << std::flush;

  Scanner scanner{std::move(preprocessed_code)};
  auto tokens{scanner.Tokenize()};
  std::ofstream tokens_file{GetFileName(file, ".txt")};
  std::transform(std::begin(tokens), std::end(tokens),
                 std::ostream_iterator<std::string>{tokens_file, "\n"},
                 std::mem_fn(&Token::ToString));
  tokens_file << std::flush;

  Parser parser{std::move(tokens)};
  auto unit{parser.ParseTranslationUnit()};
  JsonGen{}.GenJson(unit, GetFileName(file, ".html"));

  if (StandardIR) {
    std::string cmd{"clang -o standard.ll -std=c17 -S -emit-llvm -O0 " + file};
    std::system(cmd.c_str());
  }

  if (!ParseOnly) {
    CodeGen code_gen{file};
    code_gen.GenCode(unit);

    Optimization(OptimizationLevel);

    std::error_code error_code;
    llvm::raw_fd_ostream ir_file{GetFileName(file, ".ll"), error_code};
    ir_file << *Module;

    std::string cmd{"llc " + GetFileName(file, ".ll")};
    std::system(cmd.c_str());

    ObjGen(GetFileName(file, ".o"));
  }
}

void RunTest() {
  Run("testmain.c");
  InputFilePaths.erase(std::remove(std::begin(InputFilePaths),
                                   std::end(InputFilePaths), "testmain.c"),
                       std::end(InputFilePaths));

  for (const auto &file : InputFilePaths) {
    Run(file);

    if (!ParseOnly) {
      if (!Link({GetFileName(file, ".o"), "testmain.o"}, OptimizationLevel,
                GetFileName(file, ".out"))) {
        Error("link fail");
      }

      std::string cmd{"./" + GetFileName(file, ".out")};
      std::system(cmd.c_str());
    }
  }

  PrintWarnings();
}

void RunDev() {
  auto file{InputFilePaths.front()};
  Run(file);

  std::string cmd{"lli " + GetFileName(file, ".ll")};
  std::system(cmd.c_str());
}
#endif
