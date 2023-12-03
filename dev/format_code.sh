#!/usr/bin/env bash
set -euo pipefail

# Formats files according our coding style.
#
# Usage:
#
#   format_code.sh [--all] [--sudo] [--docker]
#
# Notes:
#   - Run the command "./dev/format_code.sh" in the root of the repository.
#   - If you have Docker installed (on Linux), it's recommended to pass the
#     "--docker" parameter. A docker image containing all required tools will
#     then be created and used so you don't have to install any dependencies.
#   - To run docker with sudo, use the "--sudo" parameter.
#   - Without docker, make sure the executables "git", "clang-format",
#     "cmake-format", "python3" and "xmlsort" are available in PATH.
#   - To format all files (instead of only modified ones), add the "--all"
#     parameter. This is intended only for LibrePCB maintainers, usually you
#     should not use this!

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

REPO_ROOT=$(git rev-parse --show-toplevel)

if [ "$DOCKER" == "--docker" ]; then
  DOCKER_IMAGE=librepcb/devtools:2.0.0

  if [ "$($DOCKER_CMD images -q $DOCKER_IMAGE | wc -l)" == "0" ]; then
    echo "Building devtools container..."
    $DOCKER_CMD build "$REPO_ROOT/dev/devtools" -t $DOCKER_IMAGE
  fi

  echo "[Re-running format_code.sh inside Docker container]"
  $DOCKER_CMD run --rm -t --user "$(id -u):$(id -g)" \
    -v "$REPO_ROOT:/code" \
    $DOCKER_IMAGE \
    /usr/bin/env bash -c "cd /code && dev/format_code.sh $ALL"

  echo "[Docker done.]"
  exit 0
fi

COUNTER=0

# Format source files with clang-format and Python 3.
clang_format_failed() {
  echo "" >&2
  echo "ERROR: clang-format failed!" >&2
  echo "  Make sure that clang-format 6 and Python 3 are installed." >&2
  echo "  On Linux, you can also run this script in a docker" >&2
  echo "  container by using the '--docker' argument." >&2
  exit 7
}
echo "Formatting sources with $CLANGFORMAT and Python..."
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
      NEW_CONTENT=$($CLANGFORMAT -style=file "$file" || clang_format_failed)
      NEW_CONTENT=$(echo "$NEW_CONTENT" | "$REPO_ROOT/dev/format_code_helper.py" "$file" || clang_format_failed)
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

# Format *.ui files with Python 3.
ui_format_failed() {
  echo "" >&2
  echo "ERROR: Python failed!" >&2
  echo "  Make sure that Python 3 is installed." >&2
  echo "  On Linux, you can also run this script in a docker" >&2
  echo "  container by using the '--docker' argument." >&2
  exit 7
}
echo "Formatting UI files with Python..."
for dir in apps/ libs/librepcb/ tests/unittests/
do
  if [ "$ALL" == "--all" ]; then
    TRACKED=$(git ls-files -- "${dir}**.ui")
  else
    # Only files which differ from the master branch
    TRACKED=$(git diff --name-only master -- "${dir}**.ui")
  fi
  UNTRACKED=$(git ls-files --others --exclude-standard -- "${dir}**.ui")
  for file in $TRACKED $UNTRACKED
  do
    if [ -f "$file" ]; then
      OLD_CONTENT=$(cat "$file")
      NEW_CONTENT=$(echo "$OLD_CONTENT" | "$REPO_ROOT/dev/format_code_helper.py" "$file" || ui_format_failed)
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

# Format CMake files with cmake-format.
cmake_format_failed() {
  echo "" >&2
  echo "ERROR: cmake-format failed!" >&2
  echo "  Make sure that cmake-format is installed." >&2
  echo "  On Linux, you can also run this script in a docker" >&2
  echo "  container by using the '--docker' argument." >&2
  exit 7
}
echo "Formatting CMake files with cmake-format..."
if [ "$ALL" == "--all" ]; then
  TRACKED=$(git ls-files -- "**CMakeLists.txt" "*.cmake")
else
  # Only files which differ from the master branch
  TRACKED=$(git diff --name-only master -- "**CMakeLists.txt" "*.cmake")
fi
UNTRACKED=$(git ls-files --others --exclude-standard -- "**CMakeLists.txt" "*.cmake")
for file in $TRACKED $UNTRACKED
do
  if [ -f "$file" ]; then
    OLD_CONTENT=$(cat "$file")
    NEW_CONTENT=$(cmake-format "$file" || cmake_format_failed)
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

# Format *.qrc files with xmlsort.
xmlsort_failed() {
  echo "" >&2
  echo "ERROR: xmlsort failed!" >&2
  echo "  Make sure that xmlsort is installed." >&2
  echo "  On Linux, you can also run this script in a docker" >&2
  echo "  container by using the '--docker' argument." >&2
  exit 7
}
echo "Formatting resource files with xmlsort..."
if [ "$ALL" == "--all" ]; then
  TRACKED=$(git ls-files -- "**.qrc")
else
  # Only files which differ from the master branch
  TRACKED=$(git diff --name-only master -- "**.qrc")
fi
UNTRACKED=$(git ls-files --others --exclude-standard -- "**.qrc")
for file in $TRACKED $UNTRACKED
do
  if [ -f "$file" ]; then
    OLD_CONTENT=$(cat "$file")
    NEW_CONTENT=$(xmlsort -r "RCC/qresource/file" -i -s "$file" || xmlsort_failed)
    NEW_CONTENT="${NEW_CONTENT//\'/\"}"
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

echo "Finished: $COUNTER files modified."
