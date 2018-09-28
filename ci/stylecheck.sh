#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# check if all files have Unix line endings (except 3rd-party libs)
! (git grep -Il $'\r' -- ':/' ':!/libs/clipper/')

# check if no file contains tabulators (with some exceptions)
! (git grep -Il $'\t' -- ':/' ':!/libs/clipper/' ':!/share/librepcb/licenses/' ':!*.ui')

# run python style checks
flake8 --ignore=E501 tests
