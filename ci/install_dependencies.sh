#!/usr/bin/env bash

QTIFW_VERSION="3.0.4"

QTIFW_URL_BASE="https://download.qt.io/official_releases/qt-installer-framework/$QTIFW_VERSION"
LINUXDEPLOYQT_URL="https://github.com/probonopd/linuxdeployqt/releases/download/5/linuxdeployqt-5-x86_64.AppImage"

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
    sudo apt-get install -qq qt5-default qttools5-dev-tools qtdeclarative5-dev qtdeclarative5-qtquick2-plugin
  fi
  sudo apt-get install -qq libc++-dev libglu1-mesa-dev zlib1g zlib1g-dev openssl xvfb doxygen graphviz

  # python packages
  pip install --user future flake8
  pip install --user -r ./tests/cli/requirements.txt
  pip install --user -r ./tests/funq/requirements.txt

  # linuxdeployqt
  sudo wget -cq "$LINUXDEPLOYQT_URL" -O /usr/local/bin/linuxdeployqt
  sudo chmod a+x /usr/local/bin/linuxdeployqt
  sudo ln -s ../../bin/ccache /usr/lib/ccache/clang
  sudo ln -s ../../bin/ccache /usr/lib/ccache/clang++

  # Qt Installer Framework
  wget -cq "$QTIFW_URL_BASE/QtInstallerFramework-linux-x64.run" -O ./QtIFW.run
  chmod a+x ./QtIFW.run
  ./QtIFW.run --script ./ci/qtifw-installer-noninteractive.qs --no-force-installations --platform minimal -v
  cp -rfv ~/Qt/QtIFW-$QTIFW_VERSION/bin ~/.local/

# Install dependencies on OS X
elif [ "${TRAVIS_OS_NAME-}" = "osx" ]
then

  brew update
  brew install qt5 ccache
  brew link --force qt5
  export PATH="/usr/local/opt/ccache/libexec:$PATH"

  # python packages
  pip2 install --user future flake8
  pip2 install --user -r ./tests/cli/requirements.txt
  pip2 install --user -r ./tests/funq/requirements.txt
  export PATH="$PATH:`python2 -m site --user-base`/bin"

  # Qt Installer Framework
  wget -cq "$QTIFW_URL_BASE/QtInstallerFramework-mac-x64.dmg"
  hdiutil attach ./QtInstallerFramework-mac-x64.dmg
  QTIFW_PATH="/Volumes/QtInstallerFramework-mac-x64/QtInstallerFramework-mac-x64.app/Contents/MacOS/QtInstallerFramework-mac-x64"
  chmod +x $QTIFW_PATH
  $QTIFW_PATH --script ./ci/qtifw-installer-noninteractive.qs --no-force-installations -v
  export PATH="/Users/travis/Qt/QtIFW-$QTIFW_VERSION/bin:$PATH"

  # Stop creating shit files (https://superuser.com/questions/259703/get-mac-tar-to-stop-putting-filenames-in-tar-archives)
  export COPYFILE_DISABLE=1

# Install dependencies on Windows (inside MSYS2)
elif [ -n "${APPVEYOR-}" ]
then

  # MSYS2 packages
  pacman -Sy --noconfirm --needed openssl libopenssl mingw-w64-x86_64-ccache
  ccache -s

  # python packages
  pip install future flake8
  pip install -r ./tests/cli/requirements.txt
  pip install -r ./tests/funq/requirements.txt

fi
