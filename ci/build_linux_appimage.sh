#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Build CLI AppImage
cp -r "./build/install" "./build/appimage-cli"
mv -f "./build/appimage-cli/opt/bin/librepcb-cli" "./build/appimage-cli/opt/bin/librepcb"
cp "./build/appimage-cli/opt/share/icons/hicolor/scalable/apps/org.librepcb.LibrePCB.svg" \
  "./build/appimage-cli/org.librepcb.LibrePCB.svg"
linuxdeployqt "./build/appimage-cli/opt/share/applications/org.librepcb.LibrePCB.desktop" -bundle-non-qt-libs -appimage
mv ./LibrePCB-*-x86_64.AppImage ./artifacts/nightly_builds/librepcb-cli-nightly-linux-x86_64.AppImage

# Build LibrePCB AppImage
cp -r "./build/install" "./build/appimage"
cp "./build/appimage/opt/share/icons/hicolor/scalable/apps/org.librepcb.LibrePCB.svg" "./build/appimage/org.librepcb.LibrePCB.svg"
linuxdeployqt "./build/appimage/opt/share/applications/org.librepcb.LibrePCB.desktop" -bundle-non-qt-libs -appimage
cp ./LibrePCB-*-x86_64.AppImage ./artifacts/nightly_builds/librepcb-nightly-linux-x86_64.AppImage
