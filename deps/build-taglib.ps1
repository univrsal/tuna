Set-Location -Path "taglib"
cmake -DBUILD_SHARED_LIBS=OFF `
      -DCMAKE_INSTALL_PREFIX=./install `
      -DCMAKE_C_FLAGS="-fPIC" `
      -DCMAKE_CXX_FLAGS="-fPIC" `
      -B ./build `
      -GNinja .

Set-Location -Path "build"
ninja install
