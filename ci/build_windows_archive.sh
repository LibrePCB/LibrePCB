#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

if [ "$ARCH" = "x86_64" ]
then
  WINDOWS_SYSTEM_DIR="C:/Windows/System32"
  OPENCASCADE_DIR="C:/OpenCascade/win64"
  # OPENSSL_ROOT already set in Docker image
else
  WINDOWS_SYSTEM_DIR="C:/Windows/SysWOW64"
  OPENCASCADE_DIR="C:/OpenCascade/win32"
  OPENSSL_ROOT="C:/Qt/Tools/OpenSSL/Win_x86"
fi

# Copy VC Runtime DLLs (no idea what I'm doing here, but it seems to work...)
cp -v $WINDOWS_SYSTEM_DIR/vc*140.dll ./build/install/opt/bin/
cp -v $WINDOWS_SYSTEM_DIR/msvcp140*.dll ./build/install/opt/bin/

# ZLib DLL (for Qt5, it is bundled already)
if [ "$ARCH" = "x86_64" ]
then
  cp -v $ZLIB_ROOT/bin/libzlib.dll ./build/install/opt/bin/
fi

# Copy OpenSSL DLLs (required to get HTTPS working)
cp -v $OPENSSL_ROOT/bin/lib*.dll ./build/install/opt/bin/

# Copy OpenCascade DLLs
cp -v $OPENCASCADE_DIR/gcc/bin/libTK*.dll ./build/install/opt/bin/

# Copy MinGW DLLs
cp -v "`qmake -query QT_INSTALL_PREFIX`"/bin/lib*.dll ./build/install/opt/bin/

# Copy Qt DLLs
windeployqt --compiler-runtime --force \
    --qmldir=./build/install/opt/share/librepcb/qml \
    ./build/install/opt/bin/librepcb.exe
windeployqt --compiler-runtime --force ./build/install/opt/bin/librepcb-cli.exe

# Test if the bundles are working (hopefully catching deployment issues).
./build/install/opt/bin/librepcb-cli.exe --version
./build/install/opt/bin/librepcb.exe --exit-after-startup

# Copy everything to artifacts directory for deployment
cp -r ./build/install/opt/. ./artifacts/nightly_builds/librepcb-nightly-windows-$ARCH/
