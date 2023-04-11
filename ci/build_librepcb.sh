#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# use mingw32 make on Windows
if [ "$OS" = "windows" ]
then
  MAKE="mingw32-make"
else
  MAKE="make"
fi

# default compiler
if [ -z "${CC-}" ]; then CC="gcc"; fi
if [ -z "${CXX-}" ]; then CXX="g++"; fi

# download latest translation files (just pull the i18n submodule), except on
# release branches as the translation files are already available there
if [ ! -f ./i18n/librepcb.ts ]; then
  if [ "$OS" = "windows" ]; then  # Workaround for permission error
    git config --global --add safe.directory '*'
  fi
  git -C ./i18n checkout master && git -C ./i18n pull origin master
fi

# build librepcb
echo "Using CXX=$CXX"
echo "Using CC=$CC"
mkdir -p build && pushd build
cmake .. \
  -DCMAKE_INSTALL_PREFIX=$(pwd)/install/opt \
  -DBUILD_DISALLOW_WARNINGS=1 \
  -DLIBREPCB_ENABLE_DESKTOP_INTEGRATION=1 \
  -DLIBREPCB_BUILD_AUTHOR="LibrePCB CI"
VERBOSE=1 $MAKE -j8
$MAKE install
popd

# Prepare artifacts directory
mkdir -p ./artifacts/nightly_builds
