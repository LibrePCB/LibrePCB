#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Install the Qt image formats plugin since it is not yet available in our
# Docker images. Needs to be removed when added to the images.
if [ "$OS" = "windows" ]
then
  QT_IMAGEFORMATS_URL="https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt5_5152/qt.qt5.5152.win32_mingw81/5.15.2-0-202011130602qtimageformats-Windows-Windows_7-Mingw-Windows-Windows_7-X86.7z"
  powershell -Command "Invoke-WebRequest $QT_IMAGEFORMATS_URL -OutFile 'C:/tmp.7z' -UseBasicParsing ;" \
    && 7z x C:/tmp.7z -oC:/Qt -bsp1 \
    && rm C:/tmp.7z
elif [ "$OS" = "linux" ] && [ "${DEPLOY-}" = "true" ]
then
  QT_IMAGEFORMATS_URL="https://download.qt.io/online/qtsdkrepository/linux_x64/desktop/qt5_5152/qt.qt5.5152.gcc_64/5.15.2-0-202011130601qtimageformats-Linux-RHEL_7_6-GCC-Linux-RHEL_7_6-X86_64.7z"
  wget -c "$QT_IMAGEFORMATS_URL" -O /tmp/tmp.7z
  /tmp/docker exec -t -u root mycontainer 7za x /tmp/tmp.7z -o/opt/qt
fi

# On MacOS, we have to install some dependencies since we can't use a custom
# Docker container with all tools preinstalled.
if [ "$OS" = "mac" ]
then
  # Fix crappy MacOS shit (https://github.com/actions/setup-python/issues/577).
  rm /usr/local/bin/2to3* || true
  rm /usr/local/bin/idle3* || true
  rm /usr/local/bin/pydoc3* || true
  rm /usr/local/bin/python3* || true

  # Update homebrow to avoid issues due to outdated package database. But
  # because even the update sometimes fails, let's ignore any errors with
  # "|| true" (Apple-style error handling). Maybe this way we get succussful
  # builds even if homebrow failed, which saves a lot of time and nerves.
  echo "Updating package database..."
  brew update || true

  # Actually we don't want to waste time with upgrading packages, but sometimes
  # this needs to be done to get it working (maybe it depends on the phase of
  # the moon whether this is required or not).
  echo "Upgrading packages..."
  brew upgrade || true

  # Make python3/pip3 the default
  export PATH="/usr/local/opt/python/libexec/bin:$PATH"

  # Install Qt
  echo "Installing qt5..."
  brew install --force-bottle --overwrite qt5
  echo "Linking qt5..."
  brew link --force --overwrite qt5
  export PATH="$(brew --prefix qt5)/bin:$PATH"

  # Install OpenCascade
  echo "Installing opencascade..."
  brew install --force-bottle opencascade

  # Install dylibbundler
  echo "Installing dylibbundler..."
  brew install --force-bottle dylibbundler

  # Fix macdeployqt issue (https://github.com/actions/runner-images/issues/7522)
  echo "Killing XProtect..."
  sudo pkill -9 XProtect >/dev/null || true;
  while pgrep XProtect; do sleep 3; done;
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
fi

# Install Python packages
export PIP_BREAK_SYSTEM_PACKAGES=1
pip install $PIP_USER_INSTALL -r "$DIR/../tests/cli/requirements.txt"
pip install $PIP_USER_INSTALL -r "$DIR/../tests/funq/requirements.txt"
