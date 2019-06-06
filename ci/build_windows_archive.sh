#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

cp -v "`cygpath -u \"$VCRT_DIR\"`"/*.dll     ./build/install/opt/bin/  # MS Visual C++ DLLs
cp -v /c/MinGW/bin/zlib1.dll                 ./build/install/opt/bin/  # zlib DLL
cp -v /c/OpenSSL-Win32/bin/*eay*.dll         ./build/install/opt/bin/  # OpenSSL DLLs
cp -v "`cygpath -u \"$QTDIR\"`"/bin/lib*.dll ./build/install/opt/bin/  # MinGW DLLs
windeployqt --compiler-runtime --force ./build/install/opt/bin/librepcb.exe # Qt DLLs
windeployqt --compiler-runtime --force ./build/install/opt/bin/librepcb-cli.exe # Qt DLLs
cp -r ./build/install/opt/. ./artifacts/nightly_builds/librepcb-nightly-windows-x86/
