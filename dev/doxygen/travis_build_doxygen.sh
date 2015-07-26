#!/bin/bash

doxygen Doxyfile
cd output/html
git clone -b gh-pages $DOXYGEN_REPOSITORY
cp -rf LibrePCB-Doxygen/.git .git
rm -rf LibrePCB-Doxygen
git config user.name "LibrePCB-Builder"
git config user.email "builder@librepcb.org"
git add --all
git commit --all -m "updated doxygen documentation"
git push origin gh-pages
