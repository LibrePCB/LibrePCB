#!/bin/bash

set -ev

if [ "${TRAVIS_PULL_REQUEST}" = "false" -a "${TRAVIS_BRANCH}" = "master" ]; then 
    make clean
    cd ./dev/doxygen
    doxygen Doxyfile
    git clone -b gh-pages $DOXYGEN_REPOSITORY
    cd LibrePCB-Doxygen
    git rm -r *
    cp -rf ../output/html/* .
    git config user.name "LibrePCB-Builder"
    git config user.email "builder@librepcb.org"
    git add -A
    git commit -m "updated doxygen documentation"
    git push origin gh-pages
fi

