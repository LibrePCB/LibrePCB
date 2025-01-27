#!/usr/bin/env bash

# Print environment information like versions of installed tools.
# This way we can easily see which tools are actually used on CI,
# which is very useful when investigating CI problems.

echo "=== PATH ==="
echo $PATH
echo

echo "=== Git ==="
git --version
echo

echo "=== Qt ==="
qmake --version
qmake6 --version
echo

echo "=== CMake ==="
cmake --version
echo

echo "=== GCC ==="
gcc --version
echo

echo "=== Clang ==="
clang --version
echo

echo "=== Cargo ==="
cargo --version
echo "CARGO_HOME=$CARGO_HOME"
echo

echo "=== Rust ==="
rustc --version
echo

echo "=== ccache ==="
ccache --version
ccache --show-stats
echo "CCACHE_DIR=$CCACHE_DIR"
echo

echo "=== linuxdeployqt ==="
linuxdeployqt --version
echo

echo "=== Python ==="
python --version
pip --version
pip list
echo

echo "=== Doxygen ==="
doxygen --version
echo

echo "=== OpenSSL ==="
openssl version
echo

# Always exit without error, even if the last command failed.
exit 0
