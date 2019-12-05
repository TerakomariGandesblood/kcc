//
// Created by kaiser on 2019/11/9.
//

#include "util.h"

#include <unistd.h>
#include <wait.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <iostream>

#include <llvm/Support/raw_ostream.h>

#include "error.h"

namespace kcc {

std::string GetPath() {
  constexpr std::size_t kBufSize{256};
  char buf[kBufSize];
  // 将符号链接的内容读入buf,不超过kBufSize字节.
  // 内容不以空字符终止,返回读取的字符数,返回-1表示错误
  // /proc/self/exe代表当前程序
  auto count{readlink("/proc/self/exe", buf, kBufSize)};
  if (count == -1) {
    Error("readlink error");
  }

  auto end{std::strrchr(buf, '/')};
  return std::string(buf, end - buf);
}

decltype(std::chrono::system_clock::now()) T0;

void TimingStart() { T0 = std::chrono::system_clock::now(); }

void TimingEnd(const std::string &str) {
  auto time{std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now() - T0)
                .count()};
  std::cout << str << ": ";

  if (time > 10000000) {
    time /= 1000000;
    std::cout << time << " s" << std::endl;
  } else if (time > 10000) {
    time /= 1000;
    std::cout << time << " ms" << std::endl;
  } else {
    std::cout << time << " μs" << std::endl;
  }
}

void InitCommandLine(int argc, char *argv[]) {
  llvm::cl::HideUnrelatedOptions(Category);

  llvm::cl::SetVersionPrinter([](llvm::raw_ostream &os) {
    os << "Kaiser's C Compiler\n";
    os << "Version " << KCC_VERSION << '\n';
    os << "InstalledDir: " << GetPath() << '\n';
  });

  llvm::cl::ParseCommandLineOptions(argc, argv);
}

void CommandLineCheck() {
  std::vector<std::string> files;

  for (const auto &item : InputFilePaths) {
    std::filesystem::path path{item};

    if (std::filesystem::is_directory(path)) {
      Error("Can't be a directory: {}", path.string());
    }

    if (path.filename().string() == "*.c") {
      for (const auto &file : std::filesystem::directory_iterator{
               std::filesystem::current_path()}) {
        if (file.path().filename().extension().string() == ".c") {
          files.push_back(file.path().filename().string());
        }
      }
    } else if (std::filesystem::exists(path)) {
      if (path.filename().extension().string() == ".c") {
        files.push_back(item);
      } else if (path.filename().extension().string() == ".so") {
        SoFile.push_back(item);
      } else if (path.filename().extension().string() == ".a") {
        AFile.push_back(item);
      } else if (path.filename().extension().string() == ".o") {
        ObjFile.push_back(item);
      } else {
        Error("the file extension must be '.c' or '.so': {}", item);
      }
    } else {
      Error("no such file: {}", item);
    }
  }

  if (std::size(files) == 0 && std::size(ObjFile) == 0) {
    Error("no input files");
  }

  std::sort(std::begin(files), std::end(files));
  files.erase(std::unique(std::begin(files), std::end(files)), std::end(files));

  InputFilePaths.clear();
  for (const auto &item : files) {
    InputFilePaths.push_back(item);
  }

  for (const auto &folder : IncludePaths) {
    if (!std::filesystem::exists(folder)) {
      Error("no such directory: {}", folder);
    }
  }

  if (!std::empty(OutputFilePath) && std::size(InputFilePaths) > 1 &&
      (Preprocess || OutputAssembly || OutputObjectFile || EmitTokens ||
       EmitAST || EmitLLVM)) {
    Error("Cannot specify -o when generating multiple output files");
  }

  for (auto &&item : RPath) {
    if (!std::filesystem::exists(item)) {
      Error("no such directory: {}", item);
    }

    item = "-rpath=" + item;
  }

  for (auto &&item : Libs) {
    item = "-l" + item;
  }
}

void EnsureFileExists(const std::string &file_name) {
  if (!std::filesystem::exists(file_name)) {
    Error("no such file: {}", file_name);
  }
}

std::string GetObjFile(const std::string &name) {
  auto file_name{std::filesystem::path{name + ".o"}};
  return ("/tmp" / file_name).string();
}

const std::vector<std::string> &GetObjFiles() {
  for (const auto &item : InputFilePaths) {
    ObjFile.push_back(GetObjFile(item));
  }

  for (const auto &item : InputFilePaths) {
    RemoveFile.push_back(GetObjFile(item));
  }

  return ObjFile;
}

std::string GetFileName(const std::string &name, std::string_view extension) {
  return std::filesystem::path{name}.replace_extension(extension).string();
}

void RemoveAllFiles(const std::vector<std::string> &files) {
  for (const auto &item : files) {
    std::filesystem::remove(item);
  }
}

bool CommandSuccess(std::int32_t status) {
  return status != -1 && WIFEXITED(status) && !WEXITSTATUS(status);
}

}  // namespace kcc
