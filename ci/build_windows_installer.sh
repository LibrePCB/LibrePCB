#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Bash on Windows calls the Windows provided "find" tool instead of the one
# from MSYS, which is bullshit since it isn't compatible. As a workaround,
# we adjust PATH.
export PATH="/usr/bin:$PATH"

# Build the installer.
OUTPUT_DIRECTORY=".\\artifacts\\nightly_builds"
OUTPUT_BASENAME="librepcb-installer-nightly-$OS-$ARCH"
cp -r ./dist/innosetup/* ./build/dist/innosetup/
cp -r ./build/install/. ./build/dist/innosetup/files
iscc ./build/dist/innosetup/installer.iss \
  //O"$OUTPUT_DIRECTORY" //F"$OUTPUT_BASENAME"

# Print checksum to allow fully transparent public verification.
sha256sum "$OUTPUT_DIRECTORY\\$OUTPUT_BASENAME.exe"
