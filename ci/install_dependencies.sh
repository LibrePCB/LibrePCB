#!/usr/bin/env bash

LINUXDEPLOYQT_URL="https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"

# Install dependencies on Linux
if [ "${TRAVIS_OS_NAME-}" = "linux" ]
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

  # linuxdeployqt
  sudo wget -cq "$LINUXDEPLOYQT_URL" -O /usr/local/bin/linuxdeployqt
  sudo chmod a+x /usr/local/bin/linuxdeployqt
  sudo ln -s ../../bin/ccache /usr/lib/ccache/clang
  sudo ln -s ../../bin/ccache /usr/lib/ccache/clang++

# Install dependencies on OS X
elif [ "${TRAVIS_OS_NAME-}" = "osx" ]
then

  brew update
  brew install qt5 ccache
  brew link --force qt5
  export PATH="/usr/local/opt/ccache/libexec:$PATH"

  # Stop creating shit files (https://superuser.com/questions/259703/get-mac-tar-to-stop-putting-filenames-in-tar-archives)
  export COPYFILE_DISABLE=1

# Install dependencies on Windows (inside MSYS2)
elif [ -n "${APPVEYOR-}" ]
then

  pacman -Syu --noconfirm --needed openssl

fi
