#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# On MacOS, we have to install some dependencies since we can't use a custom
# Docker container with all tools preinstalled.
if [ "$OS" = "mac" ]
then
  # Update homebrow to avoid issues due to outdated package database. But
  # because even the update sometimes fails, let's ignore any errors with
  # "|| true" (Apple-style error handling). Maybe this way we get succussful
  # builds even if homebrow failed, which saves a lot of time and nerves.
  echo "Updating package database..."
  brew update || true

  # Actually we don't want to waste time with upgrading packages, but sometimes
  # this needs to be done to get it working (maybe it depends on the phase of
  # the moon whether this is required or not).
  echo "Upgrading packages..."
  brew upgrade || true

  # Install create-dmg
  echo "Installing create-dmg..."
  brew install --force-bottle create-dmg

  # Install dylibbundler
  echo "Installing dylibbundler..."
  brew install --force-bottle dylibbundler

  # Install Ninja
  echo "Installing ninja..."
  brew install --force-bottle ninja

  # Install Rust toolchain
  # IMPORTANT: Rust from homebrew contains a stdlib that does not run on older
  # macOS versions, therefore we use rustup to install an official build!
  echo "Installing Rust toolchain..."
  brew install --force-bottle rustup
  rustup install --profile minimal 1.89.0
  export PATH="/usr/local/opt/rustup/bin:$PATH"

  # Install ccache
  echo "Installing ccache..."
  brew install --force-bottle ccache

  # Install uv
  echo "Installing uv..."
  brew install --force-bottle uv

  # Fix macdeployqt issue (https://github.com/actions/runner-images/issues/7522)
  echo "Killing XProtect..."
  sudo pkill -9 XProtect >/dev/null || true;
  while pgrep XProtect; do sleep 3; done;

  # Build Qt from source if not available from cache
  if [ ! -d "$QT_ROOT" ]; then
    echo "Qt is not available, building from source..."
    curl -o qt.tar.xz -L "$QT_URL"
    echo "$QT_SHA256  qt.tar.xz" | shasum -a 256 --check
    tar -xf qt.tar.xz
    pushd qt-everywhere-src-*
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
    popd
  fi
  export PATH="$QT_ROOT/bin:$PATH"

  # Build OpenCascade from source if not available from cache
  OCC_URL="https://github.com/Open-Cascade-SAS/OCCT/archive/refs/tags/V${OCC_VERSION}.tar.gz"
  if [ ! -d "$OCC_ROOT" ]; then
    echo "OpenCascade is not available, building from source..."
    curl -o occ.tar.gz -L "$OCC_URL"
    tar -xf occ.tar.gz
    pushd OCCT-*
    cmake . -G "Ninja" \
      -DCMAKE_BUILD_TYPE=Release \
      -DINSTALL_DIR="$OCC_ROOT" \
      -DBUILD_LIBRARY_TYPE=Shared \
      -DBUILD_DOC_Overview=0 \
      -DBUILD_MODULE_ApplicationFramework=0 \
      -DBUILD_MODULE_DataExchange=1 \
      -DBUILD_MODULE_Draw=0 \
      -DBUILD_MODULE_FoundationClasses=0 \
      -DBUILD_MODULE_ModelingAlgorithms=0 \
      -DBUILD_MODULE_ModelingData=0 \
      -DBUILD_MODULE_Visualization=0 \
      -DUSE_DRACO=0 \
      -DUSE_FREEIMAGE=0 \
      -DUSE_FREETYPE=0 \
      -DUSE_GLES2=0 \
      -DUSE_OPENGL=0 \
      -DUSE_OPENVR=0 \
      -DUSE_RAPIDJSON=0 \
      -DUSE_TBB=0 \
      -DUSE_TK=0 \
      -DUSE_VTK=0 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET"
    ninja
    ninja install
    popd
  fi
fi

# Install Python packages
export CMAKE_GENERATOR=Ninja
export FUNQ_MAKE_PATH=ninja
uv --directory "$DIR/../tests/cli" sync --no-dev
uv --directory "$DIR/../tests/funq" sync --no-dev

# If access to Slint UI testing is available, install UI testing dependencies
if [[ -n "${SLINT_UI_TESTING_AUTH-}" && ! "${SLINT_UI_TESTING_AUTH-}" =~ ^\$ ]]
then
  git config --global url."https://${SLINT_UI_TESTING_AUTH}@github.com/slint-ui/ui-testing".insteadOf "ssh://git@github.com/slint-ui/ui-testing.git"
  uv --directory "$DIR/../tests/ui" sync --no-dev
fi
