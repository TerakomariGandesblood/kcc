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
    export CC=clang
    export CXX=clang++
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

C_FLAGS=""
CXX_FLAGS=""

if $thread || $memory; then
    # libc++
    if [ ! -f "llvm-project-llvmorg-11.1.0.zip" ]; then
        wget -q https://github.com/llvm/llvm-project/archive/llvmorg-11.1.0.zip \
            -O llvm-project-llvmorg-11.1.0.zip
    fi
    unzip -q llvm-project-llvmorg-*.zip
    rm llvm-project-llvmorg-*.zip
    cd llvm-project-llvmorg-*

    if $thread; then
        cmake -S llvm -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DLLVM_ENABLE_LIBCXX=ON \
            -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi;clang;lld" -DLLVM_USE_SANITIZER=Thread
        C_FLAGS="-fsanitize=thread"
        CXX_FLAGS="-fsanitize=thread -stdlib=libc++"
    elif
        $memory
    then
        cmake -S llvm -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DLLVM_ENABLE_LIBCXX=ON \
            -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi;clang;lld" -DLLVM_USE_SANITIZER=MemoryWithOrigins
        C_FLAGS="-fsanitize=memory -fsanitize-memory-track-origins -fsanitize-memory-use-after-dtor -fno-omit-frame-pointer -fno-optimize-sibling-calls"
        CXX_FLAGS="-fsanitize=memory -fsanitize-memory-track-origins -fsanitize-memory-use-after-dtor -fno-omit-frame-pointer -fno-optimize-sibling-calls -stdlib=libc++"
    else
        cmake -S llvm -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
            -DLLVM_ENABLE_PROJECTS="clang;lld"
    fi

    if $thread || $memory; then
        cmake --build build --config $BUILD_TYPE -j$(nproc) --target cxx cxxabi clang lld
        sudo cmake --build build --config $BUILD_TYPE --target install-cxx install-cxxabi
    else
        cmake --build build --config $BUILD_TYPE -j$(nproc) --target cxx cxxabi clang lld
        sudo cmake --build build --config $BUILD_TYPE --target install-cxx install-cxxabi
    fi

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

# fmt
if [ ! -f "fmt-7.1.3.zip" ]; then
    wget -q https://github.com/fmtlib/fmt/archive/7.1.3.zip -O fmt-7.1.3.zip
fi
unzip -q fmt-*.zip
rm fmt-*.zip
cd fmt-*
cmake -S . -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DFMT_DOC=OFF -DFMT_TEST=OFF -DBUILD_SHARED_LIBS=TRUE \
    -DCMAKE_C_FLAGS="$C_FLAGS" \
    -DCMAKE_CXX_FLAGS="$CXX_FLAGS"
cmake --build build --config $BUILD_TYPE -j$(nproc)
sudo cmake --build build --config $BUILD_TYPE --target install
cd ..
echo "Build and install fmt completed"

# magic enum
if [ ! -f "magic_enum-0.7.2.zip" ]; then
    wget -q https://github.com/Neargye/magic_enum/archive/v0.7.2.zip -O magic_enum-0.7.2.zip
fi
unzip -q magic_enum-*.zip
rm magic_enum-*.zip
cd magic_enum-*
cmake -S . -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DMAGIC_ENUM_OPT_BUILD_EXAMPLES=OFF -DMAGIC_ENUM_OPT_BUILD_TESTS=OFF \
    -DCMAKE_C_FLAGS="$C_FLAGS" \
    -DCMAKE_CXX_FLAGS="$CXX_FLAGS"
cmake --build build --config $BUILD_TYPE -j$(nproc)
sudo cmake --build build --config $BUILD_TYPE --target install
cd ..
echo "Build and install magic enum completed"

# json
if [ ! -f "json-3.9.1.zip" ]; then
    wget -q https://github.com/nlohmann/json/archive/v3.9.1.zip -O json-3.9.1.zip
fi
unzip -q json-*.zip
rm json-*.zip
cd json-*
cmake -S . -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DJSON_BuildTests=OFF -DJSON_MultipleHeaders=ON \
    -DCMAKE_C_FLAGS="$C_FLAGS" \
    -DCMAKE_CXX_FLAGS="$CXX_FLAGS"
cmake --build build --config $BUILD_TYPE -j$(nproc)
sudo cmake --build build --config $BUILD_TYPE --target install
cd ..
echo "Build and install json completed"

# icu
if [ ! -f "icu4c-68_2-src.tgz" ]; then
    wget -q https://github.com/unicode-org/icu/releases/download/release-68-2/icu4c-68_2-src.tgz \
        -O icu4c-68_2-src.tgz
fi
tar -xf icu4c-*-src.tgz
rm icu4c-*-src.tgz
cd icu/source
./configure --enable-tests=no --enable-samples=no
make -j$(nproc)
sudo make install
cd ../..
echo "Build and install icu completed"

# boost
if [ ! -f "boost_1_75_0.tar.gz" ]; then
    wget -q https://dl.bintray.com/boostorg/release/1.75.0/source/boost_1_75_0.tar.gz \
        -O boost_1_75_0.tar.gz
fi
tar -xf boost_*.tar.gz
rm boost_*.tar.gz
cd boost_*
./bootstrap.sh
./b2 --toolset=gcc-10 --with-locale
sudo ./b2 --toolset=gcc-10 --with-locale install
cd ..
echo "Build and install boost completed"

cd ..

sudo ldconfig

echo "Build and install completed"
