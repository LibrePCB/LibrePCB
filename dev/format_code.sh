#!/usr/bin/env bash
set -eo pipefail

# Formats all files which differ from their state on the master branch with
# clang-format.
#
# Usage:
#   - Make sure the executables "clang-format" and "git" are available in PATH.
#   - Run the command "./dev/format_code.sh" in the root of the repository.
#   - To run clang-format in a docker-container, use the "--docker" parameter.

echo "Formatting modified files with clang-format..."

if [ "$1" == "--docker" ]; then
  DOCKER_IMAGE=librepcb/clang-format:6
  REPO_ROOT=$(git rev-parse --show-toplevel)

  if [ "$(docker images -q $DOCKER_IMAGE | wc -l)" == "0" ]; then
    echo "Building clang-format container..."
    cd "$REPO_ROOT/dev/clang-format"
    docker build . -t librepcb/clang-format:6
    cd -
  fi

  echo "[Re-running format_code.sh inside Docker container]"
  docker run --rm -t -i \
    -v "$REPO_ROOT:/code" \
    $DOCKER_IMAGE \
    /bin/bash -c "cd /code && dev/format_code.sh"

  echo "[Docker done.]"
  exit 0
fi

COUNTER=0

for dir in apps/ libs/librepcb/ tests/unittests/
do
  MODIFIED=`git diff --name-only master -- "${dir}**.cpp" "${dir}**.hpp" "${dir}**.h"`
  UNTRACKED=`git ls-files --others --exclude-standard -- "${dir}**.cpp" "${dir}**.hpp" "${dir}**.h"`
  for file in $MODIFIED $UNTRACKED
  do
    # Note: Do NOT use in-place edition of clang-format because this causes
    # "make" to detect the files as changed every time, even if the content was
    # not modified! So we only overwrite the files if their content has changed.
    OLD_CONTENT=`cat "$file"`
    NEW_CONTENT=`clang-format -style=file "$file"`
    if [ "$NEW_CONTENT" != "$OLD_CONTENT" ]
    then
        printf "%s\n" "$NEW_CONTENT" > "$file"
        echo "[M] $file"
        COUNTER=$((COUNTER+1))
    else
        echo "[ ] $file"
    fi
  done
done

echo "Finished: $COUNTER files modified."
