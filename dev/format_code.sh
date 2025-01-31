#!/usr/bin/env bash
set -eufo pipefail

# Formats files according our coding style.
#
# Usage:
#
#   format_code.sh [--all] [--sudo] [--docker] [--base <base>] [--check]
#
# Notes:
#   - Run the command "./dev/format_code.sh" in the root of the repository.
#   - If you have Docker installed (on Linux), it's recommended to pass the
#     "--docker" parameter. A docker image containing all required tools will
#     then be created and used so you don't have to install any dependencies.
#   - To run docker with sudo, use the "--sudo" parameter.
#   - Without docker, make sure the executables "git", "clang-format",
#     "cmake-format", "rustfmt", "python3" and "xmlsort" are available in PATH.
#   - To format all files (instead of only modified ones), add the "--all"
#     parameter. This is intended only for LibrePCB maintainers, usually you
#     should not use this!
#   - To update files modified against some branch <branch> in history, use
#     option "--base <base>". If this is not specified, the default "master"
#     branch will be used.
#   - To only check which files are wrongly formatted, use option "--check".
#     No file modifications will be made, nonzero exit code will be thrown
#     instead.

DOCKER=""
DOCKER_CMD="docker"
DOCKER_IMAGE="librepcb/librepcb-dev:devtools-4"
CLANGFORMAT=${CLANGFORMAT:-clang-format}
RUSTFMT=${RUSTFMT:-rustfmt}
BASE="master"
CHECK=""
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
  BASE=""
  shift
  ;;
  --base)
  shift
  BASE=$1
  shift
  ;;
  --check)
  CHECK="--check"
  shift;
  ;;
esac
done

REPO_ROOT=$(git rev-parse --show-toplevel)

if [ "$DOCKER" == "--docker" ]; then
  echo "[Re-running format_code.sh inside Docker container]"
  $DOCKER_CMD run --rm -t --user "$(id -u):$(id -g)" -v "$REPO_ROOT:/code" \
    $DOCKER_IMAGE \
    /usr/bin/env bash -c "cd /code && dev/format_code.sh --base \"$BASE\" $CHECK"

  echo "[Docker done.]"
  exit 0
fi

COUNTER=0

# This function searches for all files for a given Git match pattern.
# It searchs sorted for tracked files first, then following untracked files.
#
# All found files are put on stdout so it is parsable by foreach loop.
search_files() {
  if [ "$BASE" == "" ]; then
    TRACKED=$(git ls-files -- $@)
  else
    # Only files which differ from the base branch
    TRACKED=$(git diff --name-only ${BASE} -- $@)
  fi
  UNTRACKED=$(git ls-files --others --exclude-standard -- $@)

  for file in $TRACKED $UNTRACKED; do
    if [ -f "$file" ]; then
      echo "$file"
    fi
  done
}

# Helper to count and print a modified file
file_modified() {
  echo "[M] $1"
  COUNTER=$((COUNTER+1))
}

# Helper to print a non-modified file
file_not_modified() {
  echo "[ ] $1"
}

# This function tracks modifications of file and prints out information that
# file has been processed by the script. It increments processed file counter.
#
# Note: Do NOT use in-place edition of the tools because these causes "make"
# to detect the files as changed every time, even if the content was not
# modified! So we only overwrite the files here if their content has changed.
update_file() {
  NEW_CONTENT=$(cat)
  OLD_CONTENT=$(cat "$1")

  if [ "$NEW_CONTENT" != "$OLD_CONTENT" ]; then
    if [ "$CHECK" == "" ]; then
      printf "%s\n" "$NEW_CONTENT" > "$1"
    fi
    file_modified "$1"
  else
    if [ "$CHECK" == "" ]; then
      file_not_modified "$1"
    fi
  fi
}

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
for dir in apps/ libs/librepcb/ tests/unittests/ share/; do
  for file in $(search_files "${dir}**.cpp" "${dir}**.hpp" "${dir}**.h" "${dir}**.js"); do
    $CLANGFORMAT -style=file "$file" | "$REPO_ROOT/dev/format_code_helper.py" "$file" | update_file "$file" || clang_format_failed
  done
done

# Format rust files with rustfmt.
# Note: Currently the --check mode doesn't work properly (script aborts
# when a file is not formatted).
rustfmt_failed() {
  echo "" >&2
  echo "ERROR: rustfmt failed!" >&2
  echo "  Make sure that rustfmt is installed." >&2
  echo "  On Linux, you can also run this script in a docker" >&2
  echo "  container by using the '--docker' argument." >&2
  exit 7
}
rustfmt_process() {
  if [ -z $(cat) ]; then
    file_not_modified "$1"
  else
    file_modified "$1"
  fi
}
echo "Formatting Rust sources with $RUSTFMT..."
for file in $(search_files "*.rs"); do
  $RUSTFMT -q -l $CHECK "$file" | rustfmt_process "$file" || rustfmt_failed
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
for dir in apps/ libs/librepcb/ tests/unittests/; do
  for file in $(search_files "${dir}**.ui"); do
    cat "$file" | "$REPO_ROOT/dev/format_code_helper.py" "$file" | update_file "$file" || ui_format_failed
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
for file in $(search_files "**CMakeLists.txt" "*.cmake"); do
  cmake-format "$file" | update_file "$file" || cmake_format_failed
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
for file in $(search_files "**.qrc"); do
  xmlsort -r "RCC/qresource/file" -i -s "$file" | tr "'" '"' | update_file "$file" || xmlsort_failed
done

# Format .reuse/dep5 files with debian-copyright-sorter.
dcs_failed() {
  echo "" >&2
  echo "ERROR: debian-copyright-sorter failed!" >&2
  echo "  On Linux, you can also run this script in a docker" >&2
  echo "  container by using the '--docker' argument." >&2
  exit 7
}
if command -v debian-copyright-sorter 2>&1 >/dev/null; then
  echo "Formatting license files with debian-copyright-sorter..."
  for file in $(search_files "**/dep5"); do
    debian-copyright-sorter --iml -s casefold "$file" | update_file "$file" || dcs_failed
  done
else
  echo "Formatting license files with debian-copyright-sorter DISABLED ..."
  echo "  Make sure that debian-copyright-sorter is installed."
  echo "  On Linux, you can also run this script in a docker"
  echo "  container by using the '--docker' argument."
fi

echo "Finished: $COUNTER files modified."

# Nonzero exit code.
if [ "$CHECK" == "--check" ] && [ $COUNTER -ne 0 ]; then
  exit 1
fi
