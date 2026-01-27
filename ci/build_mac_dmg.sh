#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Build dmg
# Requirement: brew install create-dmg
pushd "./build/install/"  # Avoid having path in DMG name
for run in {1..10}; do
  if [ ! -f ./LibrePCB.dmg ]; then
    create-dmg --skip-jenkins --volname "LibrePCB" \
      --volicon ./LibrePCB.app/Contents/Resources/librepcb.icns \
      ./LibrePCB.dmg ./LibrePCB.app || true
    sleep 5
  fi
done
popd

# Test if the bundles are working (hopefully catching deployment issues).
./build/install/LibrePCB.app/Contents/MacOS/librepcb-cli --version
./build/install/LibrePCB.app/Contents/MacOS/librepcb --exit-after-startup

# Print checksums to allow fully transparent public verification.
shasum -a 256 ./build/install/LibrePCB.dmg

# Move to artifacts.
mv ./build/install/LibrePCB.dmg ./artifacts/nightly_builds/librepcb-nightly-mac-$ARCH.dmg
