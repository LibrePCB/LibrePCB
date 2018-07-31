#!/usr/bin/env bash

QTIFW_VERSION="3.0.4"
TRANSIFEX_CLI_VERSION="0.13.3"

QTIFW_URL_BASE="https://download.qt.io/official_releases/qt-installer-framework/$QTIFW_VERSION"
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
  sudo apt-get install -qq libc++-dev libglu1-mesa-dev zlib1g zlib1g-dev openssl xvfb doxygen graphviz

  # python packages
  pip install --user --upgrade --upgrade-strategy only-if-needed \
              "urllib3[secure]==1.22" "transifex-client~=$TRANSIFEX_CLI_VERSION"

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
  pip2 install --user --upgrade --upgrade-strategy only-if-needed \
               "urllib3==1.22" "transifex-client~=$TRANSIFEX_CLI_VERSION"
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

  pacman -Sy --noconfirm --needed openssl
  pip install --upgrade --upgrade-strategy only-if-needed \
              "urllib3==1.22" "transifex-client~=$TRANSIFEX_CLI_VERSION"

fi
