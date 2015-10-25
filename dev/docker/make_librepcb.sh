#!/bin/bash

mkdir -p /opt/build-librepcb-Desktop-Debug
pushd /opt/build-librepcb-Desktop-Debug
qmake -r ../LibrePCB/librepcb.pro
make -j8
make install
popd
