#!/bin/bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -eu -o pipefail

# generate doxygen HTML output
DOXYFILE=$(cat Doxyfile)
if [[ $* == *-Werror* ]]; then
  echo "Will fail on warnings."
  DOXYFILE=$(sed "s/WARN_AS_ERROR\\s*=\\s*NO/WARN_AS_ERROR = YES/g" <<< "$DOXYFILE")
fi
echo "$DOXYFILE" | doxygen -

# generate Qt helpfile
qhelpgenerator ./output/html/index.qhp -o ./output/qt_helpfile.qch
