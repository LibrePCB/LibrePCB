#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Install Python packages
export CMAKE_GENERATOR=Ninja
export FUNQ_MAKE_PATH=ninja
uv --directory "$DIR/../tests/cli" sync --no-dev
uv --directory "$DIR/../tests/funq" sync --no-dev

# If access to Slint UI testing is available, install UI testing dependencies
if [[ -n "${UV_INDEX_SLINT_PRIVATE_PASSWORD-}" && ! "${UV_INDEX_SLINT_PRIVATE_PASSWORD-}" =~ ^\$ ]]
then
  uv --directory "$DIR/../tests/ui" sync --no-dev
fi
