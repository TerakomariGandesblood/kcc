# kcc

[![Build](https://github.com/KaiserLancelot/kcc/actions/workflows/build.yml/badge.svg)](https://github.com/KaiserLancelot/kcc/actions/workflows/build.yml)
[![Coverage Status](https://coveralls.io/repos/github/KaiserLancelot/kcc/badge.svg?branch=main)](https://coveralls.io/github/KaiserLancelot/kcc?branch=main)
[![GitHub License](https://img.shields.io/github/license/KaiserLancelot/kcc)](https://raw.githubusercontent.com/KaiserLancelot/kcc/master/LICENSE)
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)
[![GitHub Releases](https://img.shields.io/github/release/KaiserLancelot/kcc)](https://github.com/KaiserLancelot/kcc/releases/latest)
[![GitHub Downloads](https://img.shields.io/github/downloads/KaiserLancelot/kcc/total)](https://github.com/KaiserLancelot/kcc/releases)
[![Bugs](https://img.shields.io/github/issues/KaiserLancelot/kcc/bug)](https://github.com/KaiserLancelot/kcc/issues?q=is%3Aopen+is%3Aissue+label%3Abug)

---

A small C11 compiler

## Environment:

- Linux
- gcc 11.1
- gcc/clang(Request to support C++17)

## Libraries:

- LLVM 12
- fmt
- magic_enum
- Boost
- libreadline-dev(for test)

## Build

```bash
cmake -S . -B build
cmake --build build --config Release -j"$(nproc)"
```

## Install

```bash
sudo cmake --build build --config Release --target install
```

## Uninstall

```bash
sudo cmake --build build --config Release --target uninstall
```

## Use

Same as gcc/clang, but only a few common command line arguments are implemented

```bash
kcc test.c -O3 -o test
```

## Reference

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
