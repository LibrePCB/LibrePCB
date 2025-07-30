#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Bash on Windows calls the Windows provided "find" tool instead of the one
# from MSYS, which is bullshit since it isn't compatible. As a workaround,
# we adjust PATH.
export PATH="/usr/bin:$PATH"

# Copy VC Runtime DLLs (no idea what I'm doing here, but it seems to work...)
cp -v C:/Windows/System32/vc*140.dll ./build/install/bin/
cp -v C:/Windows/System32/msvcp140*.dll ./build/install/bin/

# ZLib DLL
cp -v $ZLIB_ROOT/bin/libzlib.dll ./build/install/bin/

# Copy OpenSSL DLLs (required to get HTTPS working)
cp -v $OPENSSL_ROOT/bin/lib*.dll ./build/install/bin/

# Copy OpenCascade DLLs
cp -v C:/OpenCascade/win64/gcc/bin/libTK*.dll ./build/install/bin/

# Copy MinGW DLLs
cp -v "`qmake -query QT_INSTALL_PREFIX`"/bin/lib*.dll ./build/install/bin/

# Copy/move Slint DLL
cp -v ./build/install/lib/librepcbslint.dll ./build/install/bin/
cp -v ./build/install/lib/librepcbslint.dll ./build/tests/unittests/
rm -rfv ./build/install/lib

# Copy Qt DLLs
windeployqt --compiler-runtime --force ./build/install/bin/librepcb.exe
windeployqt --compiler-runtime --force ./build/install/bin/librepcb-cli.exe

# Print checksums of executables *before signing* to allow public verification.
sha256sum ./build/install/bin/*.exe

# Sign the executables (the conditional is in sign.ps1 to ensure by CI that
# the call is correct even if signing is disabled for nightly builds).
powershell.exe -File ci/sign.ps1 $(cygpath -aw ./build/install/bin/librepcb.exe)
powershell.exe -File ci/sign.ps1 $(cygpath -aw ./build/install/bin/librepcb-cli.exe)

# Test if the bundles are working (hopefully catching deployment issues).
./build/install/bin/librepcb-cli.exe --version
./build/install/bin/librepcb.exe --exit-after-startup

# Print checksums to allow fully transparent public verification.
find ./build/install -type f -exec sha256sum {} \; | sort -k 2

# Copy everything to artifacts directory for deployment
cp -r ./build/install/. ./artifacts/nightly_builds/librepcb-nightly-windows-$ARCH/
