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

# set special flag for clang (see https://github.com/travis-ci/travis-ci/issues/5383)
if [ "$CC" = "clang" ]; then CFLAGS+=" -Qunused-arguments"; fi
if [ "$CXX" = "clang++" ]; then CXXFLAGS+=" -Qunused-arguments"; fi

# use libc++ for clang on Linux (see https://stackoverflow.com/questions/24692538/)
if [ "$OS" = "linux" ] && [ "$CC" = "clang" ]; then
  CFLAGS+=" -stdlib=libc++"
  CXXFLAGS+=" -stdlib=libc++"
  BUILDSPEC="-spec linux-clang-libc++"
fi

# additional qmake arguments
if [ "${UNBUNDLE-}" != "" ]; then
  ADDITIONAL_ARGS="UNBUNDLE+=$UNBUNDLE"
else
  ADDITIONAL_ARGS=""
fi

# download latest translation files (just pull the i18n submodule), except on
# release branches as the translation files are already available there
if [ ! -f ./i18n/librepcb.ts ]; then
  git -C ./i18n checkout master && git -C ./i18n pull origin master
fi

# build librepcb
mkdir build && pushd build
qmake ../librepcb.pro -r ${BUILDSPEC-} \
  "QMAKE_CXX=$CXX" \
  "QMAKE_CC=$CC" \
  "QMAKE_CFLAGS=$CFLAGS" \
  "QMAKE_CXXFLAGS=$CXXFLAGS" \
  "PREFIX=$(pwd)/install/opt" \
  $ADDITIONAL_ARGS
$MAKE -j8
$MAKE install
popd

# Prepare artifacts directory
mkdir -p ./artifacts/nightly_builds
