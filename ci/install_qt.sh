#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Clean output directory
rm -rf "$QT_ROOT"

# Download & Extract
curl -o qt.tar.xz -L "$QT_URL"
echo "$QT_SHA256  qt.tar.xz" | shasum -a 256 --check
tar -xf qt.tar.xz
cd qt-everywhere-src-*

# Build & Install
./configure \
  -prefix "$QT_ROOT" \
  -opensource \
  -confirm-license \
  -release \
  -platform macx-clang \
  -qt-zlib -qt-libpng -qt-libjpeg \
  -nomake examples \
  -nomake tests \
  -skip qt3d \
  -skip qtactiveqt \
  -skip qtcharts \
  -skip qtcoap \
  -skip qtdatavis3d \
  -skip qtdeclarative \
  -skip qtdoc \
  -skip qtgraphs \
  -skip qtgrpc \
  -skip qthttpserver \
  -skip qtlanguageserver \
  -skip qtlocation \
  -skip qtlottie \
  -skip qtmqtt \
  -skip qtmultimedia \
  -skip qtopcua \
  -skip qtpositioning \
  -skip qtquick1 \
  -skip qtquick3d \
  -skip qtquick3dphysics \
  -skip qtquickcontrols \
  -skip qtquickeffectmaker \
  -skip qtquicktimeline \
  -skip qtremoteobjects \
  -skip qtscxml \
  -skip qtsensors \
  -skip qtserialbus \
  -skip qtserialport \
  -skip qtshadertools \
  -skip qtspeech \
  -skip qtvirtualkeyboard \
  -skip qtwayland \
  -skip qtwebchannel \
  -skip qtwebengine \
  -skip qtwebkit \
  -skip qtwebkit-examples \
  -skip qtwebsockets \
  -skip qtwebview \
  -skip qtwinextras \
  -- \
  -DQT_FORCE_WARN_APPLE_SDK_AND_XCODE_CHECK=ON \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET" \
  -DCMAKE_CXX_STANDARD=11
ninja
ninja install
