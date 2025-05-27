#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

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
  echo "Installing qt6..."
  brew install --force-bottle --overwrite qt6
  echo "Linking qt6..."
  brew link --force --overwrite qt6
  export PATH="$(brew --prefix qt6)/bin:$PATH"

  # Install OpenCascade
  echo "Installing opencascade..."
  brew install --force-bottle opencascade

  # Install create-dmg
  echo "Installing create-dmg..."
  brew install --force-bottle create-dmg

  # Install dylibbundler
  echo "Installing dylibbundler..."
  brew install --force-bottle dylibbundler

  # Install Ninja
  echo "Installing ninja..."
  brew install --force-bottle ninja

  # Install Rust toolchain
  echo "Installing Rust toolchain..."
  brew install --force-bottle rust

  # Install ccache
  echo "Installing ccache..."
  brew install --force-bottle ccache

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
export CMAKE_GENERATOR=Ninja
export FUNQ_MAKE_PATH=ninja
pip install $PIP_USER_INSTALL -r "$DIR/../tests/cli/requirements.txt"
pip install $PIP_USER_INSTALL -r "$DIR/../tests/funq/requirements.txt"
