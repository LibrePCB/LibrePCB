#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# use mingw32 make on Windows
if [ -n "${APPVEYOR-}" ]
then
  MAKE="mingw32-make"
else
  MAKE="make"
fi

# default compiler
if [ -z "${CC-}" ]; then CC="gcc"; fi
if [ -z "${CXX-}" ]; then CXX="g++"; fi

# make all warnings into errors
CFLAGS="-Werror"
CXXFLAGS="-Werror"

# set special flag for clang (see https://github.com/travis-ci/travis-ci/issues/5383)
if [ "$CC" = "clang" ]; then CFLAGS+=" -Qunused-arguments"; fi
if [ "$CXX" = "clang++" ]; then CXXFLAGS+=" -Qunused-arguments"; fi

# use libc++ for clang on Linux (see https://stackoverflow.com/questions/24692538/)
if [ "${TRAVIS_OS_NAME-}" = "linux" -a "$CC" = "clang" ]; then
  CFLAGS+=" -stdlib=libc++"
  CXXFLAGS+=" -stdlib=libc++"
  BUILDSPEC="-spec linux-clang-libc++"
fi

# download translation files from Transifex (only if API token is available)
if [ -n "${TX_TOKEN-}" ]; then tx pull --source --all --no-interactive; fi

# update translation source file
lupdate -silent -no-obsolete -source-language en -target-language en ./librepcb.pro

# build librepcb
mkdir build && pushd build
qmake ../librepcb.pro -r ${BUILDSPEC-} "QMAKE_CXX=$CXX" "QMAKE_CC=$CC" "QMAKE_CFLAGS=$CFLAGS" "QMAKE_CXXFLAGS=$CXXFLAGS" "PREFIX=`pwd`/install/opt"
$MAKE -j8
$MAKE install
popd

# Prepare artifacts directory
mkdir -p ./artifacts/nightly_builds

# Linux: Build AppImages
if [ "${TRAVIS_OS_NAME-}" = "linux" ]
then
  cp -r "./build/install" "./build/install-cli"
  mv -f "./build/install-cli/opt/bin/librepcb-cli" "./build/install-cli/opt/bin/librepcb"
  LD_LIBRARY_PATH="" linuxdeployqt "./build/install-cli/opt/share/applications/librepcb.desktop" -bundle-non-qt-libs -appimage
  mv ./LibrePCB-x86_64.AppImage ./LibrePCB-CLI-x86_64.AppImage
  LD_LIBRARY_PATH="" linuxdeployqt "./build/install/opt/share/applications/librepcb.desktop" -bundle-non-qt-libs -appimage
  if [ "${DEPLOY_APPIMAGE-}" = "true" ]
  then
    cp ./LibrePCB-x86_64.AppImage ./artifacts/nightly_builds/librepcb-nightly-linux-x86_64.AppImage
    cp ./LibrePCB-CLI-x86_64.AppImage ./artifacts/nightly_builds/librepcb-cli-nightly-linux-x86_64.AppImage
  fi
fi

# Mac: Build application bundles
if [ "${TRAVIS_OS_NAME-}" = "osx" ]
then
  # replace "bin" and "share" directories with the single *.app directory
  mv "./build/install/opt/bin/librepcb.app" "./build/install/opt/librepcb.app"
  cp -r "./build/install/opt/share" "./build/install/opt/librepcb.app/Contents/"
  mv "./build/install/opt/bin/librepcb-cli.app" "./build/install/opt/librepcb-cli.app"
  cp -r "./build/install/opt/share" "./build/install/opt/librepcb-cli.app/Contents/"
  rm -r "./build/install/opt/bin" "./build/install/opt/share"
  macdeployqt "./build/install/opt/librepcb.app" -dmg
  mv ./build/install/opt/librepcb.dmg ./librepcb.dmg
  macdeployqt "./build/install/opt/librepcb-cli.app" -dmg
  mv ./build/install/opt/librepcb-cli.dmg ./librepcb-cli.dmg
  if [ "${DEPLOY_BUNDLE-}" = "true" ]
  then
    cp ./librepcb.dmg ./artifacts/nightly_builds/librepcb-nightly-mac-x86_64.dmg
    cp ./librepcb-cli.dmg ./artifacts/nightly_builds/librepcb-cli-nightly-mac-x86_64.dmg
  fi
fi

# Windows: Copy DLLs to output directory
if [ -n "${APPVEYOR-}" ]
then
  cp -v "`cygpath -u \"$VCRT_DIR\"`"/*.dll     ./build/install/opt/bin/  # MS Visual C++ DLLs
  cp -v /c/MinGW/bin/zlib1.dll                 ./build/install/opt/bin/  # zlib DLL
  cp -v /c/OpenSSL-Win32/bin/*eay*.dll         ./build/install/opt/bin/  # OpenSSL DLLs
  cp -v "`cygpath -u \"$QTDIR\"`"/bin/lib*.dll ./build/install/opt/bin/  # MinGW DLLs
  windeployqt --compiler-runtime --force ./build/install/opt/bin/librepcb.exe # Qt DLLs
  windeployqt --compiler-runtime --force ./build/install/opt/bin/librepcb-cli.exe # Qt DLLs
  cp -r ./build/install/opt/. ./artifacts/nightly_builds/librepcb-nightly-windows-x86/
fi

# Build installer
if [ "$DEPLOY_INSTALLER" = "true" ]
then
  if [ "${TRAVIS_OS_NAME-}" = "linux" ]
  then
    TARGET="linux-x86_64"
    EXECUTABLE_EXT="run"
  elif [ "${TRAVIS_OS_NAME-}" = "osx" ]
  then
    TARGET="mac-x86_64"
    EXECUTABLE_EXT="dmg"
  elif [ -n "${APPVEYOR-}" ]
  then
    TARGET="windows-x86"
    EXECUTABLE_EXT="exe"
  fi
  ./dist/installer/update_metadata.sh "$TARGET" "0.1.0"  # TODO: How to determine version number?
  PACKAGES_DIR="./artifacts/installer_packages/$TARGET"
  mkdir -p $PACKAGES_DIR/librepcb.nightly.app/data/nightly
  cp -r ./dist/installer/output/packages/. $PACKAGES_DIR/
  cp -r ./build/install/opt/. $PACKAGES_DIR/librepcb.nightly.app/data/nightly/
  binarycreator --online-only -c ./dist/installer/output/config/config.xml -p $PACKAGES_DIR \
                ./artifacts/nightly_builds/librepcb-installer-nightly-$TARGET.$EXECUTABLE_EXT
fi

# Build Doxygen documentation
if [ "${BUILD_DOXYGEN-}" = "true" ]
then
  pushd ./dev/doxygen
  doxygen Doxyfile
  popd
  cp -r ./dev/doxygen/output/. ./artifacts/doxygen/
fi
