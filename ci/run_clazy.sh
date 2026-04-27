#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Run clazy static analysis on the C++ source code. The checks to enable are
# configured via CLAZY_CHECKS in CMakePresets.json (currently all default
# checks are disabled). BUILD_DISALLOW_WARNINGS=1 causes any clazy warning to
# be treated as a build error.
cmake --preset clazy -DBUILD_DISALLOW_WARNINGS=1
cmake --build --preset clazy
