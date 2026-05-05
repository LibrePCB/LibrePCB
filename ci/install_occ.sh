#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Clean output directory
rm -rf "$OCC_ROOT"

# Download & Extract
OCC_URL="https://github.com/Open-Cascade-SAS/OCCT/archive/refs/tags/V7_9_1.tar.gz"
curl -o occ.tar.gz -L "$OCC_URL"
echo "$OCC_SHA256  occ.tar.gz" | shasum -a 256 --check
tar -xf occ.tar.gz
cd OCCT-*

# Build & Install
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
