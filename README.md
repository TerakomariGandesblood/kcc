# kcc

[![CI](https://github.com/KaiserLancelot/kcc/workflows/CI/badge.svg)](https://github.com/KaiserLancelot/kcc/actions)
[![Coverage Status](https://coveralls.io/repos/github/KaiserLancelot/kcc/badge.svg?branch=master)](https://coveralls.io/github/KaiserLancelot/kcc?branch=master)
[![License](https://img.shields.io/github/license/KaiserLancelot/kcc)](LICENSE)
[![Badge](https://img.shields.io/badge/link-996.icu-%23FF4D5B.svg?style=flat-square)](https://996.icu/#/en_US)
[![Lines](https://tokei.rs/b1/github/KaiserLancelot/kcc)](https://github.com/Aaronepower/tokei)

A small C11 compiler

# Quick Start

#### Environment:

* Linux(I use Manjaro)
* gcc 10.1(For header files and link libraries)
* gcc/clang(Request to support C++17)

#### Libraries:

* fmt
* LLVM 10
* clang
* lld
* Qt
* Boost

#### Build

```bash
mkdir build && cd build
cmake ..
cmake --build . -j8
```

#### Install

```bash
make install
```

#### Uninstall

```bash
make uninstall
```

#### Use

Same as gcc/clang, but only a few common command line arguments are implemented

```bash
kcc test.c -O3 -o test
```

# Reference
* Library
  * https://llvm.org/docs/CommandLine.html
  * https://llvm.org/docs/GetElementPtr.html
  * https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html

* Standard document
  * http://open-std.org/JTC1/SC22/WG14/www/docs/n1570.pdf 
  * https://zh.cppreference.com/w/c/language

* Project
  * https://github.com/wgtdkp/wgtcc
  * https://github.com/rui314/8cc
