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

# make all warnings into errors
CFLAGS="-Werror"
CXXFLAGS="-Werror"

# use libc++ for clang on Linux (see https://stackoverflow.com/questions/24692538/)
if [ "$OS" = "linux" ] && [ "$CC" = "clang" ]; then
  CFLAGS+=" -stdlib=libc++"
  CXXFLAGS+=" -stdlib=libc++"
  BUILDSPEC="-spec linux-clang-libc++"
fi

# download latest translation files (just pull the i18n submodule), except on
# release branches as the translation files are already available there
if [ ! -f ./i18n/librepcb.ts ]; then
  git -C ./i18n checkout master && git -C ./i18n pull origin master
fi

# show cmake and qt versions
cmake --version
qmake --version

# build librepcb
echo "Using CXX=$CXX"
echo "Using CC=$CC"
echo "Using CFLAGS=$CFLAGS"
echo "Using CXXFLAGS=$CXXFLAGS"
mkdir -p build && pushd build
cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/install/opt
VERBOSE=1 $MAKE -j8
$MAKE install
popd

# Prepare artifacts directory
mkdir -p ./artifacts/nightly_builds
