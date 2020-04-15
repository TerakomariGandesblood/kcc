# kcc

A small C11 compiler

# Quick Start

#### Environment:

* Linux(I use Manjaro)
* gcc 9.3(For header files and link libraries)
* gcc/clang(Request to support C++17)

#### Libraries:

* fmt
* LLVM
* clang
* lld
* Qt
* Boost

#### Build

```bash
mkdir build && cd build
cmake ..
make -j8
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
  * https://github.com/cacay/MemoryPool
  * https://github.com/zhangjiantao/llvm-cxxapi
