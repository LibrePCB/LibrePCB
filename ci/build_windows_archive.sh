#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Copy VC Runtime DLLs (no idea what I'm doing here, but it seems to work...)
cp -v C:/Windows/SysWOW64/vc*140.dll ./build/install/opt/bin/
cp -v C:/Windows/SysWOW64/msvcp140*.dll ./build/install/opt/bin/

# Copy OpenSSL DLLs (required to get HTTPS working)
cp -v C:/Qt/Tools/OpenSSL/Win_x86/bin/lib*.dll ./build/install/opt/bin/

# Copy MinGW DLLs
cp -v "`qmake -query QT_INSTALL_PREFIX`"/bin/lib*.dll ./build/install/opt/bin/

# Copy Qt DLLs
windeployqt --compiler-runtime --force ./build/install/opt/bin/librepcb.exe
windeployqt --compiler-runtime --force ./build/install/opt/bin/librepcb-cli.exe

# Copy everything to artifacts directory for deployment
cp -r ./build/install/opt/. ./artifacts/nightly_builds/librepcb-nightly-windows-x86/
