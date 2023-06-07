#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

pushd ./dev/doxygen
./make.sh -Werror
popd

# Remove Qt helpfile to heavily reduce the upload file size.
rm ./dev/doxygen/output/*.qch

mkdir -p ./artifacts
cp -r ./dev/doxygen/output/. ./artifacts/doxygen/
