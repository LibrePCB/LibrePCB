#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -eufv -o pipefail

# run tests
if [ "${TRAVIS_OS_NAME-}" = "linux" ]
then
  xvfb-run -a ./build/output/qztest
  xvfb-run -a ./build/output/tests
elif [ "${TRAVIS_OS_NAME-}" = "osx" ]
then
  ./build/output/qztest
  ./build/output/tests
else
  ./build/output/qztest.exe
  ./build/output/tests.exe
fi

