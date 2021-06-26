#!/usr/bin/env bash

mkdir -p ../build && cd ../build

set -euo pipefail
IFS=$'\n\t'

HEAD=$(git rev-parse --abbrev-ref HEAD)
if [[ "${HEAD}" == "HEAD" ]]; then
  HEAD=$(git rev-parse HEAD)
fi
echo "HEAD=${HEAD}"

STASH=0
if [[ -n $(git status --porcelain) ]]; then
  git stash --include-untracked
  STASH=1
fi
echo "STASH=${STASH}"

START="master"
END=${HEAD}

trap "git checkout ${HEAD} && test ${STASH} -ne 1 || git stash pop" EXIT

readonly COMMITS=$(git log --pretty=oneline "${START}...${END}" | cut -d' ' -f1 | tac)
for commit in ${COMMITS[@]}; do
  git checkout $commit
  cmake .. > "build_${commit}.log" 2>&1
  make -j$(nproc) >> "build_${commit}.log" 2>&1
done

echo "SUCCESSFULLY FINISHED!"

