#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# On MacOS, we have to install some dependencies since we can't use a custom
# Docker container with all tools preinstalled.
if [ "$OS" = "mac" ]
then
  # Update homebrow to avoid issues due to outdated package database. But
  # because even the update sometimes fails, let's ignore any errors with
  # "|| true" (Apple-style error handling). Maybe this way we get successful
  # builds even if homebrow failed, which saves a lot of time and nerves.
  echo "Updating package database..."
  brew update || true

  # Actually we don't want to waste time with upgrading packages, but sometimes
  # this needs to be done to get it working (maybe it depends on the phase of
  # the moon whether this is required or not).
  echo "Upgrading packages..."
  brew upgrade || true

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
  # IMPORTANT: Rust from homebrew contains a stdlib that does not run on older
  # macOS versions, therefore we use rustup to install an official build!
  echo "Installing Rust toolchain..."
  brew install --force-bottle rustup
  rustup install --profile minimal 1.96.0
  rustup default 1.96.0
  export PATH="$(brew --prefix rustup)/bin:$PATH"

  # Install Cargo packages
  cargo install --force cargo-llvm-cov

  # Install ccache
  echo "Installing ccache..."
  brew install --force-bottle ccache

  # Install uv
  echo "Installing uv..."
  brew install --force-bottle uv

  # Fix macdeployqt issue (https://github.com/actions/runner-images/issues/7522)
  echo "Killing XProtect..."
  sudo pkill -9 XProtect >/dev/null || true;
  while pgrep XProtect; do sleep 3; done;

  # Add Qt to PATH
  export PATH="$QT_ROOT/bin:$PATH"
fi
