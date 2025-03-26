#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Fix deployment warning (not sure if critical).
export LANG="C.UTF-8"

# Remove unneeded SQL plugins to fix deployment issue:
# https://forum.qt.io/topic/151452/what-qt-specific-files-exactly-do-i-need-to-add-when-deploying
# Also this removes the OpenSSL 1.x dependency which we don't want to deploy.
rm -f $QTDIR/plugins/sqldrivers/libqsqlmimer.so
rm -f $QTDIR/plugins/sqldrivers/libqsqlmysql.so
rm -f $QTDIR/plugins/sqldrivers/libqsqlodbc.so
rm -f $QTDIR/plugins/sqldrivers/libqsqlpsql.so

# Prepare AppDir.
mkdir -p "./build/AppDir"
mv "./build/install" "./build/AppDir/usr"

# Build AppImage, which will also make the AppDir portable.
# Specify OpenSSL libraries manually since these runtime dependencies cannot
# be detected by linuxdeploy.
linuxdeploy-x86_64.AppImage --plugin qt \
  --library /opt/openssl/lib/libssl.so.3 \
  --library /opt/openssl/lib/libcrypto.so.3 \
  --custom-apprun ./dist/appimage/AppRun \
  --appdir ./build/AppDir \
  --output appimage
mv ./LibrePCB-x86_64.AppImage ./artifacts/nightly_builds/librepcb-nightly-linux-$ARCH.AppImage

# For the portable package, we only need the usr/ directory.
mv "./build/AppDir/usr" "./build/install"

# Test if the bundles are working (hopefully catching deployment issues).
# Doesn't work for AppImages unfortunately because of missing fuse on CI.
xvfb-run -a ./build/install/bin/librepcb-cli --version
xvfb-run -a ./build/install/bin/librepcb --exit-after-startup

# Copy to artifacts.
cp -r "./build/install" "./artifacts/nightly_builds/librepcb-nightly-linux-$ARCH"
