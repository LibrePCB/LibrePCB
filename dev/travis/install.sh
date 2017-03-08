#!/usr/bin/env bash

# install dependencies
if [ "${TRAVIS_OS_NAME}" = "linux" ]
then
  if [ -n "${QT_PPA}" ]
  then
    sudo add-apt-repository "${QT_PPA}" -y
    sudo apt-get update -qq
    sudo apt-get install -qq "${QT_BASE}base" "${QT_BASE}tools"
    source "/opt/${QT_BASE}/bin/${QT_BASE}-env.sh"
  else
    sudo apt-get update -qq
    sudo apt-get install -qq qt5-default qttools5-dev-tools
  fi
  sudo apt-get install -qq libglu1-mesa-dev zlib1g zlib1g-dev openssl xvfb doxygen graphviz
  sudo ln -s ../../bin/ccache /usr/lib/ccache/clang
  sudo ln -s ../../bin/ccache /usr/lib/ccache/clang++
elif [ "${TRAVIS_OS_NAME}" = "osx" ]
then
  brew update
  brew install qt5 ccache
  brew link --force qt5
  export PATH="/usr/local/opt/ccache/libexec:$PATH"
fi

