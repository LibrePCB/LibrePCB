#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Fix macdeployqt issue (https://github.com/actions/runner-images/issues/7522)
if [ -n "${AZURE_PIPELINES-}" ]
then
  echo "Killing XProtect..."
  sudo pkill -9 XProtect >/dev/null || true;
  while pgrep XProtect; do sleep 3; done;
fi

# Merge GUI application and CLI (https://github.com/LibrePCB/LibrePCB/issues/1358)
cp "./build/install/bin/librepcb-cli.app/Contents/MacOS/librepcb-cli" \
   "./build/install/bin/librepcb.app/Contents/MacOS/"

# Replace "bin" and "share" directories with the single *.app directory
cp -r "./build/install/bin/librepcb.app" "./build/install/LibrePCB.app"
cp -r "./build/install/share" "./build/install/LibrePCB.app/Contents/"

# Fix failure of macdeployqt according to
# https://github.com/orgs/Homebrew/discussions/2823.
# Note: The last 3 symlinks are required for QtQuick.
fix_macdeployqt () {
  ln -s -f $1 ./lib  # https://github.com/orgs/Homebrew/discussions/2823
  ln -s -f $1 ../lib
  rm -rf ../../lib && ln -s -f $1 ../../lib  # Directory already exists.
  ln -s -f $1 ../../../lib
}

# Build bundle
pushd "./build/install/"  # Avoid having path in DMG name
dylibbundler -ns -od -b \
  -x LibrePCB.app/Contents/MacOS/librepcb \
  -x LibrePCB.app/Contents/MacOS/librepcb-cli \
  -x lib/liblibrepcbslint.dylib \
  -d LibrePCB.app/Contents/Frameworks/ \
  -p @executable_path/../Frameworks/
if [ "$ARCH" = "arm64" ]
then
  # Not run on CI (yet), but here to allow running it locally on a Apple
  # Silicon Mac. Apple Silicon requires the binary to be signed, but
  # somehow macdeployqt fails on that so we have to do it manually.
  # Requirement: brew install create-dmg
  fix_macdeployqt "/opt/homebrew/lib"
  macdeployqt "LibrePCB.app" -always-overwrite \
    -executable="./LibrePCB.app/Contents/MacOS/librepcb" \
    -executable="./LibrePCB.app/Contents/MacOS/librepcb-cli" \
    -executable="./LibrePCB.app/Contents/Frameworks/liblibrepcbslint.dylib" \
    -qmldir="../../ci"
  codesign --force --deep -s - ./LibrePCB.app/Contents/MacOS/librepcb
  codesign --force --deep -s - ./LibrePCB.app/Contents/MacOS/librepcb-cli
  create-dmg --skip-jenkins --volname "LibrePCB" \
    --volicon ./LibrePCB.app/Contents/Resources/librepcb.icns \
    ./LibrePCB.dmg ./LibrePCB.app
else
  # On x86_64, directly create the *.dmg with macdeployqt.
  fix_macdeployqt "/usr/local/lib"
  # This is so crappy unstable, we have to try it several times :sob:
  for run in {1..10}; do
    if [ ! -f ./LibrePCB.dmg ]; then
      macdeployqt "LibrePCB.app" -dmg -always-overwrite \
        -executable="./LibrePCB.app/Contents/MacOS/librepcb" \
        -executable="./LibrePCB.app/Contents/MacOS/librepcb-cli" \
        -executable="./LibrePCB.app/Contents/Frameworks/liblibrepcbslint.dylib" \
        -qmldir="../../ci"
      sleep 5
    fi
  done
fi
popd

# Test if the bundles are working (hopefully catching deployment issues).
./build/install/LibrePCB.app/Contents/MacOS/librepcb-cli --version
./build/install/LibrePCB.app/Contents/MacOS/librepcb --exit-after-startup

# Move bundles to artifacts directory
mv ./build/install/LibrePCB.dmg ./artifacts/nightly_builds/librepcb-nightly-mac-$ARCH.dmg
