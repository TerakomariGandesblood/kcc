# kcc

**It can only be compiled for the time being, and the other parts are being sorted out **

[![Build](https://github.com/KaiserLancelot/kcc/workflows/Build/badge.svg)](https://github.com/KaiserLancelot/kcc/actions?query=workflow%3ABuild)
[![Coverage Status](https://coveralls.io/repos/github/KaiserLancelot/kcc/badge.svg)](https://coveralls.io/github/KaiserLancelot/kcc)
[![GitHub License](https://img.shields.io/github/license/KaiserLancelot/kcc)](https://raw.githubusercontent.com/KaiserLancelot/kcc/master/LICENSE)
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)
[![GitHub Releases](https://img.shields.io/github/release/KaiserLancelot/kcc)](https://github.com/KaiserLancelot/kcc/releases/latest)
[![GitHub Downloads](https://img.shields.io/github/downloads/KaiserLancelot/kcc/total)](https://github.com/KaiserLancelot/kcc/releases)
[![Bugs](https://img.shields.io/github/issues/KaiserLancelot/kcc/bug)](https://github.com/KaiserLancelot/kcc/issues?q=is%3Aopen+is%3Aissue+label%3Abug)

A small C11 compiler

# Quick Start

#### Environment:

- Linux
- gcc 10.2
- gcc/clang(Request to support C++17)
- libreadline-dev

#### Libraries:

- LLVM 11
- clang 11
- lld 11
- fmt
- magic_enum
- json
- Boost
- ICU

#### Build

```bash
cmake -S . -B build
cmake --build build --config Release -j$(nproc)
```

#### Install

```bash
sudo cmake --build build --config Release --target install
```

#### Use

Same as gcc/clang, but only a few common command line arguments are implemented

```bash
kcc test.c -O3 -o test
```

# Reference

- Library

  - https://llvm.org/docs/CommandLine.html
  - https://llvm.org/docs/GetElementPtr.html
  - https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html

- Standard document

  - http://open-std.org/JTC1/SC22/WG14/www/docs/n1570.pdf
  - https://zh.cppreference.com/w/c/language

- Project
  - https://github.com/wgtdkp/wgtcc
  - https://github.com/rui314/8cc
