#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# check if all files have Unix line endings (except 3rd-party libs)
(git grep -Il $'\r' -- ':/' ':!/libs/polyclipping/') && exit 1

# check if no file contains trailing spaces (with some exceptions)
(git grep -Il ' $' -- ':/' ':!/share/librepcb/licenses/' ':!*.dia') && exit 1

# check if no file contains tabulators (with some exceptions)
(git grep -Il $'\t' -- ':/' ':!/LICENSES/' ':!/libs/polyclipping/' ':!/share/librepcb/licenses/' ':!*.ui') && exit 1

# run python style checks
(flake8 --ignore=E501 dev tests) || exit 1

# run reuse checks
(reuse --suppress-deprecation lint) || exit 1
