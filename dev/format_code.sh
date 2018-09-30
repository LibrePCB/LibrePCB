#!/usr/bin/env bash

# Formats all files which differ from their state on the master branch with
# clang-format.
#
# Usage:
#   - Make sure the executables "clang-format" and "git" are available in PATH.
#   - Run the command "./dev/format_code.sh" in the root of the repository.

echo "Formatting modified files with clang-format..."

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
