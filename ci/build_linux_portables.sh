#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Fix deployment warning (not sure if critical).
export LANG="C.UTF-8"

# Remove unneeded SQL plugins to fix deployment issue:
# https://forum.qt.io/topic/151452/what-qt-specific-files-exactly-do-i-need-to-add-when-deploying
if command -v qmake6 &> /dev/null
then
  rm -f $QTDIR/plugins/sqldrivers/libqsqlmimer.so
  rm -f $QTDIR/plugins/sqldrivers/libqsqlmysql.so
fi

# Helper to extract and rebuild AppImages with appimagetool to get the static
# runtime which doesn't need libfuse2 and thus also runs on Ubuntu 22.04, see
# https://github.com/LibrePCB/LibrePCB/issues/980.
# In addition, replace AppRun by a script which allows to run the CLI.
patch_appimage () {
  ./LibrePCB-*-x86_64.AppImage --appimage-extract
  chmod -R 755 ./squashfs-root
  rm ./squashfs-root/AppRun
  cp ./dist/appimage/AppRun ./squashfs-root/
  rm ./LibrePCB-*-x86_64.AppImage
  appimagetool ./squashfs-root
  rm -rf ./squashfs-root
}

# Copy OpenSSL libraries manually since these runtime dependencies cannot
# be detected by linuxdeployqt.
LIBSSL="/opt/openssl/lib/libssl.so.3"
LIBCRYPTO="/opt/openssl/lib/libcrypto.so.3"
mkdir -p "./build/install/opt/lib"
cp -fv "$LIBSSL" "./build/install/opt/lib/"
cp -fv "$LIBCRYPTO" "./build/install/opt/lib/"

# Determine common linuxdeployqt flags
LINUXDEPLOYQT_FLAGS="-executable=./build/install/opt/lib/$(basename $LIBSSL)"
LINUXDEPLOYQT_FLAGS+=" -executable=./build/install/opt/lib/$(basename $LIBCRYPTO)"
LINUXDEPLOYQT_FLAGS+=" -bundle-non-qt-libs"

# Build AppImage.
cp -r "./build/install" "./build/appimage"
cp "./build/appimage/opt/share/icons/hicolor/scalable/apps/org.librepcb.LibrePCB.svg" \
  "./build/appimage/org.librepcb.LibrePCB.svg"
linuxdeployqt "./build/appimage/opt/share/applications/org.librepcb.LibrePCB.desktop" \
  -executable="./build/appimage/opt/bin/librepcb" \
  -executable="./build/appimage/opt/bin/librepcb-cli" \
  -qmldir="./build/appimage/opt/share/librepcb/qml" \
  $LINUXDEPLOYQT_FLAGS -appimage
patch_appimage
mv ./LibrePCB-*-x86_64.AppImage ./artifacts/nightly_builds/librepcb-nightly-linux-$ARCH.AppImage

# Run linuxdeployqt to bundle all libraries into the portable packages.
linuxdeployqt "./build/install/opt/bin/librepcb-cli" $LINUXDEPLOYQT_FLAGS -always-overwrite
linuxdeployqt "./build/install/opt/bin/librepcb" $LINUXDEPLOYQT_FLAGS -always-overwrite \
  -qmldir="./build/install/opt/share/librepcb/qml"

# Test if the bundles are working (hopefully catching deployment issues).
# Doesn't work for AppImages unfortunately because of missing fuse on CI.
xvfb-run -a ./build/install/opt/bin/librepcb-cli --version
xvfb-run -a ./build/install/opt/bin/librepcb --exit-after-startup

# Copy to artifacts.
cp -r "./build/install/opt" "./artifacts/nightly_builds/librepcb-nightly-linux-$ARCH"
