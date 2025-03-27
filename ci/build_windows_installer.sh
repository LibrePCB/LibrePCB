#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Bash on Windows calls the Windows provided "find" tool instead of the one
# from MSYS, which is bullshit since it isn't compatible. As a workaround,
# we adjust PATH.
export PATH="/usr/bin:$PATH"

TARGET="$OS-$ARCH"

cp -r ./dist/innosetup/* ./build/dist/innosetup/
cp -r ./build/install/. ./build/dist/innosetup/files
iscc ./build/dist/innosetup/installer.iss \
  //O".\\artifacts\\nightly_builds" \
  //F"librepcb-installer-nightly-$TARGET"
