#!/bin/bash

set -ev

if [ "${TRAVIS_PULL_REQUEST}" = "false" ]; then 
    echo "Build doxygen documentation for branch ${TRAVIS_BRANCH}..."
    make clean
    cd ./dev/doxygen
    doxygen Doxyfile
    git clone -b gh-pages $DOXYGEN_REPOSITORY
    mkdir "LibrePCB-Doxygen/${TRAVIS_BRANCH}"
    cd "LibrePCB-Doxygen/${TRAVIS_BRANCH}"
    git rm -r *
    cp -rf ../../output/html/* .
    git config user.name "LibrePCB-Builder"
    git config user.email "builder@librepcb.org"
    git add -A
    git commit -m "updated doxygen documentation of branch ${TRAVIS_BRANCH}"
    git push origin gh-pages
else
    echo "Pull request -> Do not build doxygen documentation"
fi

