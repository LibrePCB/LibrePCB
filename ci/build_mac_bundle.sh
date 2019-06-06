#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# replace "bin" and "share" directories with the single *.app directory
mv "./build/install/opt/bin/librepcb.app" "./build/install/opt/librepcb.app"
cp -r "./build/install/opt/share" "./build/install/opt/librepcb.app/Contents/"
mv "./build/install/opt/bin/librepcb-cli.app" "./build/install/opt/librepcb-cli.app"
cp -r "./build/install/opt/share" "./build/install/opt/librepcb-cli.app/Contents/"
rm -r "./build/install/opt/bin" "./build/install/opt/share"

# Build LibrePCB bundle
macdeployqt "./build/install/opt/librepcb.app" -dmg
mv ./build/install/opt/librepcb.dmg ./librepcb.dmg

# Build LibrePCB CLI bundle
macdeployqt "./build/install/opt/librepcb-cli.app" -dmg
mv ./build/install/opt/librepcb-cli.dmg ./librepcb-cli.dmg

# Copy bundles to artifacts directory
cp ./librepcb.dmg ./artifacts/nightly_builds/librepcb-nightly-mac-x86_64.dmg
cp ./librepcb-cli.dmg ./artifacts/nightly_builds/librepcb-cli-nightly-mac-x86_64.dmg
