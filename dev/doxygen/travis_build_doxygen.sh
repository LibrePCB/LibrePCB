#!/bin/bash

set -e # The -e flag causes the script to exit as soon as one command returns a non-zero exit code.

if [ "${TRAVIS_PULL_REQUEST}" = "false" ]; then 
    echo "Build doxygen documentation for branch ${TRAVIS_BRANCH}..."
    
    echo "make clean:"
    make clean
    
    echo "doxygen Doxyfile:"
    cd ./dev/doxygen
    doxygen Doxyfile
    
    echo "git clone -b gh-pages:"
    git clone -b gh-pages $DOXYGEN_REPOSITORY
    cd LibrePCB-Doxygen
    
    echo "rm -rf ${TRAVIS_BRANCH}/*:"
    mkdir -p ${TRAVIS_BRANCH}
    rm -rf ${TRAVIS_BRANCH}/*
    
    echo "git add/commit/push:"
    cp -rf ../output/html/* ./${TRAVIS_BRANCH}
    git config user.name "LibrePCB-Builder"
    git config user.email "builder@librepcb.org"
    git add -A
    git commit -m "updated doxygen documentation of branch ${TRAVIS_BRANCH}"
    git push origin gh-pages
else
    echo "Pull request -> Do not build doxygen documentation"
fi

