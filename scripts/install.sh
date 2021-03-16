#!/bin/bash

set -e

source $(dirname "$0")/install-system.sh

BUILD_TYPE=Release

while getopts 'g' OPT; do
    case $OPT in
    g)
        export CC=clang
        export CXX=clang++
        BUILD_TYPE=RelWithDebInfo
        ;;
    ?)
        exit 1
        ;;
    esac
done

if [ ! -d "dependencies" ]; then
    echo "The dependencies directory does not exist"
    exit 1
fi

cd dependencies

cd lcov-*
sudo make install
cd ..
echo "Install lcov completed"

if [ -d llvm-project-llvmorg-* ]; then
    cd llvm-project-llvmorg-*
    sudo cmake --build build --config RelWithDebInfo --target install-cxx install-cxxabi
    cd ..
    echo "Install libc++ completed"
fi

cd googletest-release-*
sudo cmake --build build --config $BUILD_TYPE --target install
cd ..
echo "Install google test completed"

# NOTE Add new dependency here

cd ..

sudo ldconfig

echo "Install completed"
