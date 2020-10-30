#!/bin/bash

set -e

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    sudo add-apt-repository ppa:ubuntu-toolchain-r/ppa
    sudo apt update
    sudo apt install gcc-10 g++-10 llvm clang-tidy valgrind \
        llvm-dev libclang-dev liblld-11-dev libreadline-dev

    export CC=gcc-10
    export CXX=g++-10
else
    echo "The system does not support: $OSTYPE"
    exit 1
fi
