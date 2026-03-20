#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# default compiler
if [ -z "${CC-}" ]; then CC="gcc"; fi
if [ -z "${CXX-}" ]; then CXX="g++"; fi

# Sanity check that we don't use clang from homebrew since it causes troubles.
# See https://github.com/actions/runner-images/issues/13827.
if $CC --version | grep -i -q "homebrew"; then
  echo "Error: Compiler seems to be coming from homebrew" >&2
  exit 1
fi

# download latest translation files (just pull the i18n submodule)
if [ "$OS" = "windows" ]; then  # Workaround for permission error
  git config --global --add safe.directory '*'
fi
git -C ./i18n checkout master && git -C ./i18n pull origin master

# fail on Slint compiler warnings
export SLINT_COMPILER_DENY_WARNINGS=1

# build librepcb
echo "Using CXX=$CXX"
echo "Using CC=$CC"
mkdir -p build && pushd build
cmake \
  -G Ninja \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_INSTALL_PREFIX=$(pwd)/install \
  -DBUILD_DISALLOW_WARNINGS=1 \
  -DLIBREPCB_ENABLE_DESKTOP_INTEGRATION=1 \
  -DLIBREPCB_BUILD_AUTHOR="$LIBREPCB_BUILD_AUTHOR" \
  ${CMAKE_OPTIONS-} \
  ..
ninja
ninja install
popd

# Prepare artifacts directory
mkdir -p ./artifacts/nightly_builds
