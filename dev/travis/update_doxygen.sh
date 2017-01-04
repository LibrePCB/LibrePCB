#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -eufv -o pipefail

# run doxygen and upload html output to https://github.com/LibrePCB/LibrePCB-Doxygen
if [ "${BUILD_DOXYGEN}" = "true" -a "${TRAVIS_PULL_REQUEST}" = "false" -a -n "${DOXYGEN_ACCESS_TOKEN-}" ]
then 
  BRANCH_NAME=$(echo ${TRAVIS_BRANCH} | sed -e 's/[^A-Za-z0-9._-]/_/g')
  cd ./dev/doxygen
  doxygen Doxyfile
  git clone -b master "https://LibrePCB-Builder:${DOXYGEN_ACCESS_TOKEN}@github.com/LibrePCB/LibrePCB-Doxygen.git"
  cd LibrePCB-Doxygen
  rm -rf $BRANCH_NAME
  cp -rf ../output/html ./$BRANCH_NAME
  git config user.name "LibrePCB-Builder"
  git config user.email "builder@librepcb.org"
  git add -A > /dev/null
  git commit --amend -q -m "Update documentation of branch '$BRANCH_NAME'"
  git push --force -q origin master
fi

