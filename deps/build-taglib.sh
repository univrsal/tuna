#!/bin/bash
cd taglib
cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_C_FLAGS="-fPIC" -DCMAKE_CXX_FLAGS="-fPIC" -DCMAKE_INSTALL_PREFIX=./install -B ./build -GNinja .
cd build
ninja install
