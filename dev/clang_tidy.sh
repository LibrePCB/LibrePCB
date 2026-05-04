#!/usr/bin/env bash
set -eufo pipefail

# Usage:
#
#  ./clang_tidy.sh [--executable=/path/to/clang-tidy] [--fix]
#
# It is highly recommended to have Docker installed. The script will then
# automatically run clang-tidy in a Docker container for a deterministic
# behavior, and with ctcache (clang-tidy-cache) being used for massive speedup.

DOCKER_IMAGE="librepcb/librepcb-dev:ubuntu-26.04-1"
DOCKER_WRAPPED_ARGS=""
TIDY_ARGS=""
EXECUTABLE="clang-tidy"
for i in "$@"
do
case $i in
  --executable)
  shift
  EXECUTABLE="$1"
  shift
  ;;
  --fix)
  DOCKER_WRAPPED_ARGS+="--fix "
  TIDY_ARGS+="-fix "
  shift;
  ;;
esac
done

REPO_ROOT=$(git rev-parse --show-toplevel)
BUILD_DIR="$REPO_ROOT/build/clang-tidy"

# If Docker is available, run the script inside a container
if command -v docker &> /dev/null; then
  echo "[Running inside Docker container $DOCKER_IMAGE]"
  docker run --rm -i -u "$(id -u):$(id -g)" \
    -v "$REPO_ROOT:/work" -w "/work" \
    -e "CARGO_HOME=/work/build/clang-tidy/.cargo-home" \
    -e "CTCACHE_DIR=/work/build/clang-tidy/.ctcache" $DOCKER_IMAGE \
    dev/clang_tidy.sh --executable "ctcache" $DOCKER_WRAPPED_ARGS
  exit $?
fi

# Build compile_commands.json, ui.h and ui_*.h header files
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
if [ ! -f compile_commands.json ]; then
  CC=clang CXX=clang++ cmake -G Ninja ../../ -DCMAKE_EXPORT_COMPILE_COMMANDS=1
fi
ninja libs/librepcb/ui/ui.h
LC_ALL="C.UTF-8" find ../../ -name '*.ui' -exec sh -c \
  '/usr/lib/qt6/libexec/uic "$1" -o "libs/librepcb/editor/ui_$(basename "$1" .ui).h"' _ {} \;

# Workaround for ExcludeHeaderFilterRegex not working as expected :-/
sed -i 's/Tag tag;/Tag tag = {};/g' \
  "$BUILD_DIR/libs/slint/generated_include/private/slint_image_internal.h"

# Run clang-tidy
cd "$REPO_ROOT"
run-clang-tidy -quiet -load=ClazyClangTidy.so \
  -export-fixes "$BUILD_DIR/fixes.yml" \
  -p "$BUILD_DIR" -clang-tidy-binary "$EXECUTABLE" \
  $TIDY_ARGS $(git ls-files -- ':*.cpp' ':!/libs/polyclipping/')
echo "Success"
