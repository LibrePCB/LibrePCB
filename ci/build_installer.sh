#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Bash on Windows calls the Windows provided "find" tool instead of the one
# from MSYS, which is bullshit since it isn't compatible. As a workaround,
# we adjust PATH.
export PATH="/usr/bin:$PATH"

if [ "$OS" = "linux" ]
then
  EXECUTABLE_EXT="run"
elif [ "$OS" = "mac" ]
then
  EXECUTABLE_EXT="dmg"
elif [ "$OS" = "windows" ]
then
  EXECUTABLE_EXT="exe"
fi
TARGET="$OS-$ARCH"
./dist/installer/update_metadata.sh "$TARGET" "0.2.0-1"  # TODO: How to determine version number?
PACKAGES_DIR="./artifacts/installer_packages/$TARGET"
mkdir -p $PACKAGES_DIR/librepcb.nightly.app/data/nightly
cp -r ./dist/installer/output/packages/. $PACKAGES_DIR/
cp -r ./build/install/opt/. $PACKAGES_DIR/librepcb.nightly.app/data/nightly/
binarycreator --online-only -c ./dist/installer/output/config/config.xml -p $PACKAGES_DIR \
              ./artifacts/nightly_builds/librepcb-installer-nightly-$TARGET.$EXECUTABLE_EXT
