#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Copy VC Runtime DLLs (no idea what I'm doing here, but it seems to work...)
cp -v C:/Windows/System32/vc*140.dll ./build/install/opt/bin/
cp -v C:/Windows/System32/msvcp140*.dll ./build/install/opt/bin/

# ZLib DLL (for Qt5, it is bundled already)
if [ "$ARCH" = "x86_64" ]
then
  cp -v $ZLIB_ROOT/bin/libzlib.dll ./build/install/opt/bin/
fi

# Copy OpenSSL DLLs (required to get HTTPS working)
cp -v $OPENSSL_ROOT/bin/lib*.dll ./build/install/opt/bin/

# Copy OpenCascade DLLs
cp -v C:/OpenCascade/win64/gcc/bin/libTK*.dll ./build/install/opt/bin/

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
