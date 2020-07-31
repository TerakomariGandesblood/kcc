//
// Created by kaiser on 2019/10/30.
//

#include "cpp.h"

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/PreprocessorOutputOptions.h>
#include <clang/Frontend/Utils.h>
#include <clang/Lex/DirectoryLookup.h>
#include <fmt/format.h>
#include <llvm/Support/raw_ostream.h>

#include "error.h"
#include "llvm_common.h"

namespace kcc {

Preprocessor::Preprocessor() {
  pp_ = &Ci.getPreprocessor();
  header_search_ = &pp_->getHeaderSearchInfo();

  AddIncludePath("/usr/include", true);
  AddIncludePath("/usr/local/include", true);

  /*
   * Platform Specific Code
   */
  AddIncludePath("/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/include", true);
  AddIncludePath("/usr/lib/gcc/x86_64-pc-linux-gnu/10.1.0/include-fixed", true);
  /*
   * End of Platform Specific Code
   */

  pp_->setPredefines(pp_->getPredefines() +
                     "#define __KCC__ 1\n"
                     "#define __GNUC__ 10\n"
                     "#define __GNUC_MINOR__ 1\n"
                     "#define __GNUC_PATCHLEVEL__ 0\n"
                     "#define __STDC_NO_ATOMICS__ 1\n"
                     "#define __STDC_NO_COMPLEX__ 1\n"
                     "#define __STDC_NO_THREADS__ 1\n"
                     "#define __STDC_NO_VLA__ 1\n"
                     "#define __builtin_va_arg(args,type) "
                     "  *(type*)__builtin_va_arg_sub(args,type)\n");
}

void Preprocessor::AddIncludePaths(
    const std::vector<std::string> &include_paths) {
  for (const auto &path : include_paths) {
    AddIncludePath(path, false);
  }
}

void Preprocessor::AddMacroDefinitions(
    const std::vector<std::string> &macro_definitions) {
  for (const auto &macro : macro_definitions) {
    auto pos{macro.find_first_of('=')};
    auto name{macro.substr(0, pos)};

    if (pos == std::string::npos) {
      pp_->setPredefines(pp_->getPredefines() +
                         fmt::format(fmt("#define {} 1\n"), name));
    } else {
      auto value{macro.substr(pos + 1)};
      pp_->setPredefines(pp_->getPredefines() +
                         fmt::format(fmt("#define {} {}\n"), name, value));
    }
  }
}

std::string Preprocessor::Cpp(const std::string &input_file) {
  Module->setSourceFileName(input_file);

  auto file{Ci.getFileManager().getFileRef(input_file).get()};
  Ci.getSourceManager().setMainFileID(Ci.getSourceManager().createFileID(
      file, clang::SourceLocation(), clang::SrcMgr::C_User));

  Ci.getDiagnosticClient().BeginSourceFile(Ci.getLangOpts(), pp_);

  std::string code;
  code.reserve(Preprocessor::StrReserve);
  llvm::raw_string_ostream os{code};

  clang::PreprocessorOutputOptions opts;
  opts.ShowCPP = true;

  clang::DoPrintPreprocessedInput(*pp_, &os, opts);
  os.flush();

  if (Ci.getDiagnostics().hasErrorOccurred()) {
    Error("Preprocess failure");
  }

  Ci.getDiagnosticClient().EndSourceFile();

  return code;
}

void Preprocessor::AddIncludePath(const std::string &path, bool is_system) {
  if (is_system) {
    clang::DirectoryLookup directory{
        Ci.getFileManager().getDirectoryRef(path).get(),
        clang::SrcMgr::C_System, false};
    header_search_->AddSearchPath(directory, true);
  } else {
    clang::DirectoryLookup directory{
        Ci.getFileManager().getDirectoryRef(path).get(), clang::SrcMgr::C_User,
        false};
    header_search_->AddSearchPath(directory, false);
  }
}

} // namespace kcc
