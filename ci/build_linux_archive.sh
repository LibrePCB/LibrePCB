#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# run linuxdeployqt to bundle Qt libs
linuxdeployqt "./build/install/opt/bin/librepcb-cli" -bundle-non-qt-libs -always-overwrite
linuxdeployqt "./build/install/opt/bin/librepcb" -bundle-non-qt-libs -always-overwrite

# copy to artifacts
cp -r "./build/install/opt" "./artifacts/nightly_builds/librepcb-nightly-linux-x86_64"
