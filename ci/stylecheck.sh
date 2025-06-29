#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# check if all files have Unix line endings (except 3rd-party libs)
(git grep -Il $'\r' -- ':/' ':!/libs/polyclipping/') && exit 1

# check if no file contains trailing spaces (with some exceptions)
(git grep -Il ' $' -- ':/' ':!/share/librepcb/licenses/' ':!*.dia') && exit 1

# check if no file contains tabulators (with some exceptions)
(git grep -Il $'\t' -- ':/' ':!/LICENSES/' ':!/libs/polyclipping/' ':!/share/librepcb/licenses/' ':!*.ui') && exit 1

# check rust code formatting
for f in $(git ls-files -- '*.rs'); do
  (rustfmt --check "$f") || exit 1
done

# lint rust crates
for f in $(git ls-files -- '**Cargo.toml'); do
  (cargo clippy --manifest-path="$f" --features="fail-on-warnings") || exit 1
done

# check slint code formatting
for f in $(git ls-files -- '*.slint'); do
  slint-lsp format -i "$f"
done
(git diff --exit-code -- "*.slint") || exit 1

# run python style checks
(uv --directory tests/cli run --only-dev ruff format --check) || exit 1
(uv --directory tests/funq run --only-dev ruff format --check) || exit 1
(uv --directory tests/ui run --only-dev ruff format --check) || exit 1

# run python linters
(flake8 --ignore=E501 dev) || exit 1
(uv --directory tests/cli run --only-dev ruff check) || exit 1
(uv --directory tests/funq run --only-dev ruff check) || exit 1
(uv --directory tests/ui run --only-dev ruff check) || exit 1

# run reuse checks
(reuse --suppress-deprecation lint) || exit 1

# check formatting of .reuse/dep5
(debian-copyright-sorter --iml -s casefold -o ".reuse/dep5" ".reuse/dep5") || exit 1
(git diff --exit-code -- ".reuse/dep5") || exit 1

# validate AppStream files
appstream-util validate share/metainfo/org.librepcb.LibrePCB.metainfo.xml
