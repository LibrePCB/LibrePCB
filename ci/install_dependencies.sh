#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

QTIFW_VERSION="3.2.2"
QTIFW_URL_BASE="https://download.qt.io/official_releases/qt-installer-framework/$QTIFW_VERSION"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Install dependencies on Linux
if [ "$OS" = "linux" ]
then

  # python packages
  pip install --user -r "$DIR/../tests/cli/requirements.txt"
  pip install --user -r "$DIR/../tests/funq/requirements.txt"
  export PATH="$PATH:`python -m site --user-base`/bin"

# Install dependencies on OS X
elif [ "$OS" = "mac" ]
then

  brew update
  brew install qt5
  brew link --force qt5

  # python packages
  pip2 install --user future "flake8==3.7.7"
  pip2 install --user -r ./tests/cli/requirements.txt
  pip2 install --user -r ./tests/funq/requirements.txt
  export PATH="$PATH:`python2 -m site --user-base`/bin"

  # Qt Installer Framework
  wget -cq "$QTIFW_URL_BASE/QtInstallerFramework-mac-x64.dmg"
  hdiutil attach ./QtInstallerFramework-mac-x64.dmg
  QTIFW_PATH="/Volumes/QtInstallerFramework-mac-x64/QtInstallerFramework-mac-x64.app/Contents/MacOS/QtInstallerFramework-mac-x64"
  chmod +x $QTIFW_PATH
  $QTIFW_PATH --script ./ci/qtifw-installer-noninteractive.qs --no-force-installations -v
  export PATH="/Users/$USER/Qt/QtIFW-$QTIFW_VERSION/bin:$PATH"

# Install dependencies on Windows (inside MSYS2)
elif [ "$OS" = "windows" ]
then

  # python packages
  export FUNQ_MAKE_PATH="mingw32-make"
  pip install future "flake8==3.7.7"
  pip install -r ./tests/cli/requirements.txt
  pip install -r ./tests/funq/requirements.txt

fi
