#!/usr/bin/bash

cmake -G Ninja . -B /build/lingodb \
    -DMLIR_DIR=/build/llvm/lib/cmake/mlir \
    -DLLVM_EXTERNAL_LIT=/build/llvm/bin/llvm-lit \
    -DArrow_DIR="/build/arrow/install/lib/cmake/arrow" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release

cmake --build /build/lingodb -j$(nproc)

# Add the following lines to the vscode workspace setting:
   # "cmake.generator": "Ninja",
   # "cmake.buildDirectory": "/build/lingodb",
   # "cmake.configureArgs": [
   #    "-DMLIR_DIR=/build/llvm/lib/cmake/mlir",
   #    "-DLLVM_EXTERNAL_LIT=/build/llvm/bin/llvm-lit",
   #    "-DArrow_DIR=/build/arrow/install/lib/cmake/arrow",
   #    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
   #    "-DCMAKE_BUILD_TYPE=Release"
   # ]