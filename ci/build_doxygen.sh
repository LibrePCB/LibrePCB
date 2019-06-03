#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

pushd ./dev/doxygen
doxygen Doxyfile
popd

mkdir -p ./artifacts
cp -r ./dev/doxygen/output/. ./artifacts/doxygen/
