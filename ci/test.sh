#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -eufv -o pipefail

# run unit tests
if [ "${TRAVIS_OS_NAME-}" = "linux" ]
then
  xvfb-run -a ./build/output/qztest
  xvfb-run -a ./build/output/librepcb-unittests
elif [ "${TRAVIS_OS_NAME-}" = "osx" ]
then
  ./build/output/qztest
  ./build/output/librepcb-unittests
else
  ./build/output/qztest.exe
  ./build/output/librepcb-unittests.exe
fi

# check if the CLI works
if [ "${TRAVIS_OS_NAME-}" = "linux" ]
then
  xvfb-run -a ./LibrePCB-CLI-x86_64.AppImage --help
elif [ "${TRAVIS_OS_NAME-}" = "osx" ]
then
  ./build/install/opt/librepcb-cli.app/Contents/MacOS/librepcb-cli --help
else
  ./build/install/opt/bin/librepcb-cli.exe --help
fi

