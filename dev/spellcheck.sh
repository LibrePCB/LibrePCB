#!/usr/bin/env bash
set -eufo pipefail

# Usage:
#
#  ./spellcheck.sh      # Check Only
#  ./spellcheck.sh -w   # Apply Fixes

git ls-files \
  --exclude-standard $(git submodule status | awk '{print ":(exclude)" $2}') \
  -- ':/' \
  ':!*.dia' \
  ':!*.dxf' \
  ':!*.icns' \
  ':!*.jpg' \
  ':!*.png' \
  ':!*.svg' \
  ':!/.gitbugtraq' \
  ':!/AUTHORS.md' \
  ':!/dev/doxygen/Doxyfile' \
  ':!/libs/polyclipping/' \
  ':!/LICENSES/' \
  ':!/share/librepcb/licenses/' \
  | uvx typos==1.46.0 --file-list - $@
