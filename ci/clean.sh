#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Helper script for local use to clean artifacts without requiring a rebuild,
# intended for debugging deployment issues.
#
# Usage:
#  1. Delete ./build_backup and ./artifacts_backup manually
#  2. Run ./ci/build_librepcb.sh
#  3. Run ./ci/clean.sh
#  4. Run & debug deployment - run /ci/clean.sh after each iteration

if [ -d "./build_backup" ]
then
  rm -rf ./build
  cp -r ./build_backup ./build
else
  cp -r ./build ./build_backup
fi

if [ -d "./artifacts_backup" ]
then
  rm -rf ./artifacts
  cp -r ./artifacts_backup ./artifacts
else
  cp -r ./artifacts ./artifacts_backup
fi
