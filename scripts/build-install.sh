#!/bin/bash

set -e

source $(dirname "$0")/install-system.sh

thread=false
memory=false
BUILD_TYPE=Release

while getopts 'mt' OPT; do
    case $OPT in
    m)
        memory=true
        ;;
    t)
        thread=true
        ;;
    ?)
        exit 1
        ;;
    esac
done

if $thread && $memory; then
    echo "error"
    exit 1
fi

if $thread || $memory; then
    export CC=clang-10
    export CXX=clang++-10
    BUILD_TYPE=RelWithDebInfo
fi

if [ ! -d "dependencies" ]; then
    echo "mkdir dependencies"
    mkdir dependencies
fi

cd dependencies

# lcov
if [ ! -f "lcov-1.15.zip" ]; then
    wget -q https://github.com/linux-test-project/lcov/archive/v1.15.zip -O lcov-1.15.zip
fi
unzip -q lcov-*.zip
rm lcov-*.zip
cd lcov-*
sudo make install
cd ..
echo "Build and install lcov completed"

# doxygen
if [ ! -f "doxygen-Release_1_8_20.zip" ]; then
    wget -q https://github.com/doxygen/doxygen/archive/Release_1_8_20.zip \
        -O doxygen-Release_1_8_20.zip
fi
unzip -q doxygen-Release_*.zip
rm doxygen-Release_*.zip
cd doxygen-Release_*
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)
sudo cmake --build build --config Release --target install
cd ..
echo "Build and install doxygen completed"

# google benchmark
if [ ! -f "benchmark-1.5.2.zip" ]; then
    wget -q https://github.com/google/benchmark/archive/v1.5.2.zip -O benchmark-1.5.2.zip
fi
unzip -q benchmark-*.zip
rm benchmark-*.zip
cd benchmark-*
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DBENCHMARK_ENABLE_TESTING=OFF -DBENCHMARK_ENABLE_LTO=ON \
    -DBENCHMARK_USE_LIBCXX=OFF -DBENCHMARK_ENABLE_GTEST_TESTS=OFF \
    -DBENCHMARK_ENABLE_ASSEMBLY_TESTS=OFF -DBUILD_SHARED_LIBS=ON
cmake --build build --config Release -j$(nproc)
sudo cmake --build build --config Release --target install
cd ..
echo "Build and install google benchmark completed"

C_FLAGS=""
CXX_FLAGS=""

if $thread || $memory; then
    # libc++
    if [ ! -f "llvm-project-llvmorg-10.0.0.zip" ]; then
        wget -q https://github.com/llvm/llvm-project/archive/llvmorg-10.0.0.zip \
            -O llvm-project-llvmorg-10.0.0.zip
    fi
    unzip -q llvm-project-llvmorg-*.zip
    rm llvm-project-llvmorg-*.zip
    cd llvm-project-llvmorg-*

    if $thread; then
        C_FLAGS="-fsanitize=thread"
        CXX_FLAGS="-fsanitize=thread -stdlib=libc++"
        cmake -S llvm -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_ENABLE_LIBCXX=ON \
            -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi" -DLLVM_USE_SANITIZER=Thread

    fi
    if $memory; then
        C_FLAGS="-fsanitize=memory -fsanitize-memory-track-origins -fsanitize-memory-use-after-dtor -fno-omit-frame-pointer -fno-optimize-sibling-calls"
        CXX_FLAGS="-fsanitize=memory -fsanitize-memory-track-origins -fsanitize-memory-use-after-dtor -fno-omit-frame-pointer -fno-optimize-sibling-calls -stdlib=libc++"
        cmake -S llvm -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_ENABLE_LIBCXX=ON \
            -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi" -DLLVM_USE_SANITIZER=MemoryWithOrigins
    fi

    cmake --build build --config RelWithDebInfo -j$(nproc) --target cxx cxxabi
    sudo cmake --build build --config RelWithDebInfo --target install-cxx install-cxxabi
    cd ..
    echo "Build and install libc++ completed"
fi

# google test
if [ ! -f "googletest-release-1.10.0.zip" ]; then
    wget -q https://github.com/google/googletest/archive/release-1.10.0.zip \
        -O googletest-release-1.10.0.zip
fi
unzip -q googletest-release-*.zip
rm googletest-release-*.zip
cd googletest-release-*
cmake -S . -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DBUILD_GMOCK=OFF -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_C_FLAGS="$C_FLAGS" \
    -DCMAKE_CXX_FLAGS="$CXX_FLAGS"
cmake --build build --config $BUILD_TYPE -j$(nproc)
sudo cmake --build build --config $BUILD_TYPE --target install
cd ..
echo "Build and install google test completed"

# NOTE Add new dependency here

cd ..

sudo ldconfig

echo "Build and install completed"
