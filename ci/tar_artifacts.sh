#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# Bash on Windows calls the Windows provided "tar" and "openssl" tool instead
# of the one from MSYS, which is bullshit since it isn't compatible. As a
# workaround, we adjust PATH.
export PATH="/usr/bin:$PATH"

# create tarball of all artifacts
cd ./artifacts
tar -cf artifacts.tar *
