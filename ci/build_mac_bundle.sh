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
cp "./build/install/opt/bin/librepcb-cli.app/Contents/MacOS/librepcb-cli" \
   "./build/install/opt/bin/librepcb.app/Contents/MacOS/"

# Replace "bin" and "share" directories with the single *.app directory
cp -r "./build/install/opt/bin/librepcb.app" "./build/install/opt/LibrePCB.app"
cp -r "./build/install/opt/share" "./build/install/opt/LibrePCB.app/Contents/"

# Build bundle
pushd "./build/install/opt/"  # Avoid having path in DMG name
dylibbundler -ns -od -b \
  -x LibrePCB.app/Contents/MacOS/librepcb \
  -x LibrePCB.app/Contents/MacOS/librepcb-cli \
  -d LibrePCB.app/Contents/Frameworks/ \
  -p @executable_path/../Frameworks/
if [ "$ARCH" = "arm64" ]
then
  # Not run on CI (yet), but here to allow running it locally on a Apple
  # Silicon Mac. Apple Silicon requires the binary to be signed, but
  # somehow macdeployqt fails on that so we have to do it manually.
  # Requirement: brew install create-dmg
  ln -s /opt/homebrew/lib ./lib  # https://github.com/orgs/Homebrew/discussions/2823
  macdeployqt "LibrePCB.app" -always-overwrite \
    -executable="./LibrePCB.app/Contents/MacOS/librepcb" \
    -executable="./LibrePCB.app/Contents/MacOS/librepcb-cli"
  codesign --force --deep -s - ./LibrePCB.app/Contents/MacOS/librepcb
  codesign --force --deep -s - ./LibrePCB.app/Contents/MacOS/librepcb-cli
  create-dmg --skip-jenkins --volname "LibrePCB" \
    --volicon ./LibrePCB.app/Contents/Resources/librepcb.icns \
    ./LibrePCB.dmg ./LibrePCB.app
else
  # On x86_64, directly create the *.dmg with macdeployqt.
  ln -s /usr/local/lib ./lib  # https://github.com/orgs/Homebrew/discussions/2823
  macdeployqt "LibrePCB.app" -dmg -always-overwrite \
    -executable="./LibrePCB.app/Contents/MacOS/librepcb" \
    -executable="./LibrePCB.app/Contents/MacOS/librepcb-cli"
fi
popd

# Test if the bundles are working (hopefully catching deployment issues).
./build/install/opt/LibrePCB.app/Contents/MacOS/librepcb-cli --version
./build/install/opt/LibrePCB.app/Contents/MacOS/librepcb --exit-after-startup

# Move bundles to artifacts directory
mv ./build/install/opt/LibrePCB.dmg ./artifacts/nightly_builds/librepcb-nightly-mac-$ARCH.dmg
