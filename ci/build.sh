#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# default compiler
if [ -z "${CC-}" ]; then CC="gcc"; fi
if [ -z "${CXX-}" ]; then CXX="g++"; fi

# make all warnings into errors
CFLAGS="-Werror"
CXXFLAGS="-Werror"

# set special flag for clang (see https://github.com/travis-ci/travis-ci/issues/5383)
if [ "$CC" = "clang" ]; then CFLAGS+=" -Qunused-arguments"; fi
if [ "$CXX" = "clang++" ]; then CXXFLAGS+=" -Qunused-arguments"; fi

# build librepcb
mkdir build && pushd build
qmake ../librepcb.pro -r "QMAKE_CXX=$CXX" "QMAKE_CC=$CC" "QMAKE_CFLAGS=$CFLAGS" "QMAKE_CXXFLAGS=$CXXFLAGS" "PREFIX=`pwd`/install/opt"
make -j8
make install
popd

# Prepare artifacts directory
mkdir -p ./artifacts/nightly_builds

# Linux: Build AppImage
if [ "${TRAVIS_OS_NAME-}" = "linux" ]
then
  LD_LIBRARY_PATH="" linuxdeployqt "./build/install/opt/share/applications/librepcb.desktop" -bundle-non-qt-libs -appimage
  if [ "${DEPLOY_APPIMAGE-}" = "true" ]
  then
    cp ./LibrePCB-x86_64.AppImage ./artifacts/nightly_builds/librepcb-nightly-linux-x86_64.AppImage
  fi
fi

# Mac: Build application bundle
if [ "${TRAVIS_OS_NAME-}" = "osx" ]
then
  macdeployqt "./build/install/opt/bin/librepcb.app" -dmg
  if [ "${DEPLOY_BUNDLE-}" = "true" ]
  then
    cp ./build/install/opt/bin/librepcb.dmg ./artifacts/nightly_builds/librepcb-nightly-mac-x86_64.dmg
  fi
fi

# Build Doxygen documentation
if [ "${BUILD_DOXYGEN-}" = "true" ]
then
  pushd ./dev/doxygen
  doxygen Doxyfile
  popd
  cp -r ./dev/doxygen/output/. ./artifacts/doxygen/
fi
