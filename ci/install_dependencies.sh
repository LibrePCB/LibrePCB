#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# On MacOS, we have to install some dependencies since we can't use a custom
# Docker container with all tools preinstalled.
if [ "$OS" = "mac" ]
then
  # Update homebrow to avoid issues due to outdated package database. But
  # because even the update sometimes fails, let's ignore any errors with
  # "|| true" (Apple-style error handling). Maybe this way we get succussful
  # builds even if homebrow failed, which saves a lot of time and nerves.
  brew update || true

  # Make python3/pip3 the default
  export PATH="/usr/local/opt/python/libexec/bin:$PATH"

  # Install Qt
  brew install qt5
  brew link --force qt5

  # Install Qt Installer Framework
  QTIFW_VERSION="3.2.2"
  QTIFW_URL_BASE="https://download.qt.io/official_releases/qt-installer-framework/$QTIFW_VERSION"
  wget -cq "$QTIFW_URL_BASE/QtInstallerFramework-mac-x64.dmg"
  hdiutil attach ./QtInstallerFramework-mac-x64.dmg
  QTIFW_PATH="/Volumes/QtInstallerFramework-mac-x64/QtInstallerFramework-mac-x64.app/Contents/MacOS/QtInstallerFramework-mac-x64"
  chmod +x $QTIFW_PATH
  $QTIFW_PATH --script ./ci/qtifw-installer-noninteractive.qs --no-force-installations -v
  export PATH="/Users/$USER/Qt/QtIFW-$QTIFW_VERSION/bin:$PATH"
fi

# Configure pip
if [ "$OS" != "windows" ]
then
  # We're not root, thus no permissions to install packages globally
  PIP_USER_INSTALL="--user"
  export PATH="$PATH:`python -m site --user-base`/bin"
else
  # Extending PATH does not work, thus installing Python packages globally
  PIP_USER_INSTALL=""
  # Set custom "make" command for building Funq
  export FUNQ_MAKE_PATH="mingw32-make"
fi

# Install Python packages
pip install $PIP_USER_INSTALL -r "$DIR/../tests/cli/requirements.txt"
pip install $PIP_USER_INSTALL -r "$DIR/../tests/funq/requirements.txt"
