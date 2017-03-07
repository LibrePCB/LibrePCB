#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -eufv -o pipefail

# build AppImage
if [ "${DEPLOY_APPIMAGE-}" = "true" ]
then
  mkdir -p ./build/librepcb.AppDir/opt/bin
  cp -r ./build/generated/share ./build/librepcb.AppDir/opt/
  cp ./build/generated/unix/librepcb ./build/librepcb.AppDir/opt/bin/librepcb
  cp ./build/librepcb.AppDir/opt/share/pixmaps/librepcb.svg ./build/librepcb.AppDir/librepcb.svg
  cp ./build/librepcb.AppDir/opt/share/applications/librepcb.desktop ./build/librepcb.AppDir/librepcb.desktop
  LD_LIBRARY_PATH="" linuxdeployqt ./build/librepcb.AppDir/opt/bin/librepcb -appimage
fi

