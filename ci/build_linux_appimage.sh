#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

cp -r "./build/install" "./build/install-cli"
mv -f "./build/install-cli/opt/bin/librepcb-cli" "./build/install-cli/opt/bin/librepcb"
linuxdeployqt "./build/install-cli/opt/share/applications/org.librepcb.LibrePCB.desktop" -bundle-non-qt-libs -appimage
mv ./LibrePCB-x86_64.AppImage ./LibrePCB-CLI-x86_64.AppImage
linuxdeployqt "./build/install/opt/share/applications/org.librepcb.LibrePCB.desktop" -bundle-non-qt-libs -appimage
cp ./LibrePCB-x86_64.AppImage ./artifacts/nightly_builds/librepcb-nightly-linux-x86_64.AppImage
cp ./LibrePCB-CLI-x86_64.AppImage ./artifacts/nightly_builds/librepcb-cli-nightly-linux-x86_64.AppImage
