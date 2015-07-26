#!/bin/bash

set -e # The -e flag causes the script to exit as soon as one command returns a non-zero exit code.

if [ "${TRAVIS_PULL_REQUEST}" = "false" ]; then 

    BRANCH_NAME=$(echo ${TRAVIS_BRANCH} | sed -e 's/[^A-Za-z0-9._-]/_/g')
    echo "Build doxygen documentation for branch $BRANCH_NAME..."
    
    echo "make clean:"
    make clean
    
    echo "doxygen Doxyfile:"
    cd ./dev/doxygen
    doxygen Doxyfile
    
    echo "git clone -b gh-pages:"
    git clone -b gh-pages $DOXYGEN_REPOSITORY
    cd LibrePCB-Doxygen
    
    echo "rm -rf $BRANCH_NAME/*:"
    mkdir -p $BRANCH_NAME
    rm -rf $BRANCH_NAME/*
    
    echo "git add/commit/push:"
    cp -rf ../output/html/* ./$BRANCH_NAME
    git config user.name "LibrePCB-Builder"
    git config user.email "builder@librepcb.org"
    git add -A > /dev/null
    git commit -m "updated doxygen documentation of branch $BRANCH_NAME"
    git push origin gh-pages
else
    echo "Pull request -> Do not build doxygen documentation"
fi

