#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# On MacOS, we have to install some dependencies since we can't use a custom
# Docker container with all tools preinstalled.
if [ "$OS" = "mac" ]
then
  # Important: Do not install any packages from Homebrew! It has caused a lot
  # of headaches in the past - life is better without it.

  # Install create-dmg
  echo "Installing create-dmg..."
  mkdir -p "$RUNNER_TEMP/create-dmg"
  curl -L "https://github.com/create-dmg/create-dmg/archive/a2b71d0fda6d0df2a86dc7f67082d4d73e84c59f.tar.gz" \
    |  tar -xz --strip-components=1 -C "$RUNNER_TEMP/create-dmg"
  make -j$(nproc) -C "$RUNNER_TEMP/create-dmg" prefix="$HOME/.local" install

  # Install dylibbundler
  echo "Installing dylibbundler..."
  mkdir -p "$RUNNER_TEMP/dylibbundler"
  curl -L "https://github.com/auriamg/macdylibbundler/archive/63105a5571e0e9a83a8f2c37d0b91f2398b2031b.tar.gz" \
    | tar -xz --strip-components=1 -C "$RUNNER_TEMP/dylibbundler"
  make -j$(nproc) -C "$RUNNER_TEMP/dylibbundler" PREFIX="$HOME/.local" install

  # Install Rust toolchain
  # IMPORTANT: Rust from homebrew contains a stdlib that does not run on older
  # macOS versions, therefore we use rustup to install an official build!
  echo "Installing Rust toolchain..."
  rustup install --profile minimal 1.96.0
  rustup default 1.96.0
  export PATH="$CARGO_HOME/bin:$PATH"

  # Install Cargo packages
  cargo install --force cargo-llvm-cov

  # Install ccache
  echo "Installing ccache..."
  mkdir -p "$RUNNER_TEMP/ccache"
  curl -o "$RUNNER_TEMP/ccache.tar.gz" -L \
    "https://github.com/ccache/ccache/releases/download/v4.14/ccache-4.14-darwin.tar.gz"
  shasum -a 256 --check <<< \
    "353a81ea8680d93387102cfde288ebed381a54272cfdc18224f241add6332b39  $RUNNER_TEMP/ccache.tar.gz"
  tar -xf "$RUNNER_TEMP/ccache.tar.gz" --strip-components=1 -C "$RUNNER_TEMP/ccache"
  "$RUNNER_TEMP/ccache/install.sh" --prefix="$HOME/.local"

  # Install uv
  echo "Installing uv..."
  curl -o "$RUNNER_TEMP/uv.tar.gz" -L "$UV_URL"
  shasum -a 256 --check <<< "$UV_SHA256  $RUNNER_TEMP/uv.tar.gz"
  mkdir -p "$HOME/.local/bin"
  tar -xf "$RUNNER_TEMP/uv.tar.gz" --strip-components=1 -C "$HOME/.local/bin"

  # Fix macdeployqt issue (https://github.com/actions/runner-images/issues/7522)
  echo "Killing XProtect..."
  sudo pkill -9 XProtect >/dev/null || true;
  while pgrep XProtect; do sleep 3; done;

  # Add Qt & tools to PATH
  export PATH="$QT_ROOT/bin:$HOME/.local/bin:$PATH"
fi
