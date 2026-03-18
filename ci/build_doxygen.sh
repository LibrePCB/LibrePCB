#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Build Doxygen documentation
pushd ./dev/doxygen
./make.sh -Werror
popd

# Build Rust documentation
files=$(git ls-files -- '**Cargo.toml')
for f in $files; do
  cargo doc --manifest-path="$f" --target-dir="./dev/doxygen/output/rust/" \
    --no-deps --document-private-items --features="fail-on-warnings,ffi"
  cp -rf ./dev/doxygen/output/rust/doc/* ./dev/doxygen/output/html/
  rm -rf ./dev/doxygen/output/rust
done

mkdir -p ./artifacts
cp -r ./dev/doxygen/output/. ./artifacts/doxygen/
