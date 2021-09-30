#!/usr/bin/env bash
set -euo pipefail

# Formats files according our coding style with clang-format.
#
# Usage:
#
#   format_code.sh [--all] [--sudo] [--docker]
#
# Notes:
#   - Make sure the executables "clang-format" and "git" are available in PATH.
#   - Run the command "./dev/format_code.sh" in the root of the repository.
#   - To run clang-format in a docker-container, use the "--docker" parameter.
#   - To run docker with sudo, use the "--sudo" parameter.
#   - To format all files (instead of only modified ones), add the "--all"
#     parameter. This is intended only for LibrePCB maintainers, don't use it!

DOCKER=""
DOCKER_CMD="docker"
CLANGFORMAT=${CLANGFORMAT:-clang-format}
ALL=""
for i in "$@"
do
case $i in
  --docker)
  DOCKER="--docker"
  shift
  ;;
  --sudo)
  DOCKER_CMD="sudo docker"
  shift
  ;;
  --all)
  ALL="--all"
  shift
  ;;
esac
done

format_failed() {
  echo "" >&2
  echo "ERROR: clang-format failed." >&2
  echo "  Make sure that clang-format 6 is installed." >&2
  echo "  You can also run clang-format in a docker container" >&2
  echo "  by using the '--docker' argument when invoking this script." >&2
  exit 7
}

echo "Formatting files with $CLANGFORMAT..."

REPO_ROOT=$(git rev-parse --show-toplevel)

if [ "$DOCKER" == "--docker" ]; then
  DOCKER_IMAGE=librepcb/clang-format:6

  if [ "$($DOCKER_CMD images -q $DOCKER_IMAGE | wc -l)" == "0" ]; then
    echo "Building clang-format container..."
    $DOCKER_CMD build "$REPO_ROOT/dev/clang-format" -t librepcb/clang-format:6
  fi

  echo "[Re-running format_code.sh inside Docker container]"
  $DOCKER_CMD run --rm -t --user "$(id -u):$(id -g)" \
    -v "$REPO_ROOT:/code" \
    $DOCKER_IMAGE \
    /bin/bash -c "cd /code && dev/format_code.sh $ALL"

  echo "[Docker done.]"
  exit 0
fi

COUNTER=0

for dir in apps/ libs/librepcb/ tests/unittests/
do
  if [ "$ALL" == "--all" ]; then
    TRACKED=$(git ls-files -- "${dir}**.cpp" "${dir}**.hpp" "${dir}**.h")
  else
    # Only files which differ from the master branch
    TRACKED=$(git diff --name-only master -- "${dir}**.cpp" "${dir}**.hpp" "${dir}**.h")
  fi
  UNTRACKED=$(git ls-files --others --exclude-standard -- "${dir}**.cpp" "${dir}**.hpp" "${dir}**.h")
  for file in $TRACKED $UNTRACKED
  do
    if [ -f "$file" ]; then
      # Note: Do NOT use in-place edition of clang-format because this causes
      # "make" to detect the files as changed every time, even if the content was
      # not modified! So we only overwrite the files if their content has changed.
      OLD_CONTENT=$(cat "$file")
      NEW_CONTENT=$($CLANGFORMAT -style=file "$file" || format_failed)
      if [ "$NEW_CONTENT" != "$OLD_CONTENT" ]
      then
        printf "%s\n" "$NEW_CONTENT" > "$file"
        echo "[M] $file"
        COUNTER=$((COUNTER+1))
      else
        echo "[ ] $file"
      fi
    fi
  done
done

echo "Finished: $COUNTER files modified."
