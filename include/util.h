//
// Created by kaiser on 2019/11/9.
//

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <llvm/Support/CommandLine.h>

namespace kcc {

enum class OptLevel { kO0, kO1, kO2, kO3 };

enum class Langs { kC };

enum class LangStds { kC89, kC99, kC11, kC17, kGnu89, kGnu99, kGnu11, kGnu17 };

inline std::vector<std::string> ObjFile;

inline std::vector<std::string> SoFile;

inline std::vector<std::string> AFile;

inline std::vector<std::string> RemoveFile;

inline llvm::cl::OptionCategory Category{"Compiler Options"};

inline llvm::cl::list<std::string> InputFilePaths{llvm::cl::desc{"input files"},
                                                  llvm::cl::Positional,
                                                  llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> OutputObjectFile{
    "c", llvm::cl::desc{"Only run preprocess, compile, and assemble steps"},
    llvm::cl::cat{Category}};

inline llvm::cl::list<std::string> MacroDefines{
    "D", llvm::cl::desc{"Predefine the specified macro"},
    llvm::cl::value_desc{"macro"}, llvm::cl::Prefix, llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> EmitTokens{
    "emit-token",
    llvm::cl::desc{"Run preprocessor, dump internal rep of tokens"},
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> EmitAST{
    "emit-ast", llvm::cl::desc{"Build ASTs and then debug dump them"},
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> EmitLLVM{
    "emit-llvm",
    llvm::cl::desc{"Build ASTs then convert to LLVM, emit .ll file"},
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> Preprocess{
    "E", llvm::cl::desc{"Run preprocessor, emit preprocessed file"},
    llvm::cl::cat{Category}};

inline llvm::cl::list<std::string> IncludePaths{
    "I", llvm::cl::desc{"Add directory to include search path"},
    llvm::cl::value_desc{"directory"}, llvm::cl::Prefix,
    llvm::cl::cat{Category}};

inline llvm::cl::opt<std::string> OutputFilePath{
    "o", llvm::cl::desc("Specify output file"), llvm::cl::value_desc("path"),
    llvm::cl::cat{Category}};

inline llvm::cl::opt<OptLevel> OptimizationLevel{
    llvm::cl::desc{"Set optimization level to <number>"},
    llvm::cl::value_desc{"number"}, llvm::cl::init(OptLevel::kO0),
    llvm::cl::values(
        clEnumValN(OptLevel::kO0, "O0", "No optimizations (default)"),
        clEnumValN(OptLevel::kO1, "O1", "Enable trivial optimizations"),
        clEnumValN(OptLevel::kO2, "O2", "Enable default optimizations"),
        clEnumValN(OptLevel::kO3, "O3", "Enable expensive optimizations")),
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> OutputAssembly{
    "S", llvm::cl::desc{"Emit native assembly code"}, llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> Timing{
    "t", llvm::cl::desc{"Print the amount of time"}, llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> Shared{"shared",
                                  llvm::cl::desc{"Generate dynamic library"},
                                  llvm::cl::cat{Category}};

inline llvm::cl::list<std::string> RPath{
    "rpath", llvm::cl::desc{"Add a DT_RUNPATH to the output"},
    llvm::cl::value_desc{"directory"}, llvm::cl::Prefix,
    llvm::cl::cat{Category}};

inline llvm::cl::list<std::string> Libs{
    "l", llvm::cl::desc{"Add library"}, llvm::cl::value_desc{"file"},
    llvm::cl::Prefix, llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> Version{"v", llvm::cl::desc{"Version Information"},
                                   llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> Debug{
    "g", llvm::cl::desc{"Generate source-level debug information"},
    llvm::cl::cat{Category}};

// 忽略
inline llvm::cl::opt<LangStds> LangStd{
    "std",
    llvm::cl::desc{"Language standard to compile for"},
    llvm::cl::init(LangStds::kGnu17),
    llvm::cl::Prefix,
    llvm::cl::values(
        clEnumValN(LangStds::kC89, "c89", "ISO C 1989"),
        clEnumValN(LangStds::kC99, "c99", "ISO C 1999"),
        clEnumValN(LangStds::kC11, "c11", "ISO C 2011"),
        clEnumValN(LangStds::kC17, "c17", "ISO C 2017"),
        clEnumValN(LangStds::kGnu89, "gnu89", "ISO C 1989 with GNU extensions"),
        clEnumValN(LangStds::kGnu99, "gnu99", "ISO C 1999 with GNU extensions"),
        clEnumValN(LangStds::kGnu11, "gnu11", "ISO C 2011 with GNU extensions"),
        clEnumValN(LangStds::kGnu17, "gnu17",
                   "ISO C 2017 with GNU extensions (default)")),
    llvm::cl::cat{Category}};

inline llvm::cl::opt<Langs> Lang{
    "x",
    llvm::cl::desc{"Specify language"},
    llvm::cl::init(Langs::kC),
    llvm::cl::Prefix,
    llvm::cl::values(clEnumValN(Langs::kC, "c", "C")),
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> FPic{
    "fPIC", llvm::cl::desc{"Emit position-independent code"},
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> FPch{
    "fpch-preprocess",
    llvm::cl::desc{"Allows use of a precompiled header together with -E"},
    llvm::cl::cat{Category}};

inline llvm::cl::opt<bool> DD{
    "dD",
    llvm::cl::desc{
        "make debugging dumps during compilation as specified by letters"},
    llvm::cl::cat{Category}};

#ifdef DEV
inline llvm::cl::opt<bool> DevMode{"dev", llvm::cl::desc{"Dev Mode"},
                                   llvm::cl::cat{Category}};
#endif

void InitCommandLine(int argc, char *argv[]);

void CommandLineCheck();

std::string GetPath();

void TimingStart();

void TimingEnd(const std::string &str = "");

void EnsureFileExists(const std::string &file_name);

std::string GetObjFile(const std::string &name);

std::string GetFileName(const std::string &name, std::string_view extension);

void RemoveFiles();

bool CommandSuccess(std::int32_t status);

bool DoNotLink();

} // namespace kcc
