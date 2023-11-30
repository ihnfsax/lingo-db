#!/usr/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 debug/release"
    exit 1
fi

if [ $1 == "debug" ]; then
    BUILD_TYPE=Debug
    BUILD_DIR=build_debug
elif [ $1 == "release" ]; then
    BUILD_TYPE=Release
    BUILD_DIR=build_release
else
    echo "Usage: $0 debug/release"
    exit 1
fi

cmake -G Ninja . -B ./${BUILD_DIR} \
    -DMLIR_DIR=/home/ihnfsa/Develop/lingo-db/llvm-project/build/lib/cmake/mlir \
    -DLLVM_EXTERNAL_LIT=/home/ihnfsa/Develop/lingo-db/llvm-project/build/bin/llvm-lit \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DLLVM_PARALLEL_LINK_JOBS=2

rm -rf ./build
ln -s ./${BUILD_DIR} ./build

cmake --build ./build -j$(nproc)
