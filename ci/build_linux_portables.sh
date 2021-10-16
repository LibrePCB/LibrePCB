#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Copy OpenSSL libraries manually since these runtime dependencies cannot
# be detected by linuxdeployqt.
mkdir -p "./build/install/opt/lib"
cp -f "/usr/lib/libssl.so.1.1" "./build/install/opt/lib/"
cp -f "/usr/lib/libcrypto.so.1.1" "./build/install/opt/lib/"

# Determine common linuxdeployqt flags
LINUXDEPLOYQT_FLAGS="-executable=./build/install/opt/lib/libssl.so.1.1"
LINUXDEPLOYQT_FLAGS+=" -executable=./build/install/opt/lib/libcrypto.so.1.1"
LINUXDEPLOYQT_FLAGS+=" -bundle-non-qt-libs"

# Build CLI AppImage.
cp -r "./build/install" "./build/appimage-cli"
mv -f "./build/appimage-cli/opt/bin/librepcb-cli" "./build/appimage-cli/opt/bin/librepcb"
cp "./build/appimage-cli/opt/share/icons/hicolor/scalable/apps/org.librepcb.LibrePCB.svg" \
  "./build/appimage-cli/org.librepcb.LibrePCB.svg"
linuxdeployqt "./build/appimage-cli/opt/share/applications/org.librepcb.LibrePCB.desktop" \
  $LINUXDEPLOYQT_FLAGS -appimage
mv ./LibrePCB-*-x86_64.AppImage ./artifacts/nightly_builds/librepcb-cli-nightly-linux-x86_64.AppImage

# Build LibrePCB AppImage.
cp -r "./build/install" "./build/appimage"
cp "./build/appimage/opt/share/icons/hicolor/scalable/apps/org.librepcb.LibrePCB.svg" \
  "./build/appimage/org.librepcb.LibrePCB.svg"
linuxdeployqt "./build/appimage/opt/share/applications/org.librepcb.LibrePCB.desktop" \
  $LINUXDEPLOYQT_FLAGS -appimage
cp ./LibrePCB-*-x86_64.AppImage ./artifacts/nightly_builds/librepcb-nightly-linux-x86_64.AppImage

# Run linuxdeployqt to bundle all libraries into the portable packages.
linuxdeployqt "./build/install/opt/bin/librepcb-cli" $LINUXDEPLOYQT_FLAGS -always-overwrite
linuxdeployqt "./build/install/opt/bin/librepcb" $LINUXDEPLOYQT_FLAGS -always-overwrite

# Copy to artifacts.
cp -r "./build/install/opt" "./artifacts/nightly_builds/librepcb-nightly-linux-x86_64"
