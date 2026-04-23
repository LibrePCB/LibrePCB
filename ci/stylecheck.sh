#!/usr/bin/env bash

# set shell settings (see https://sipb.mit.edu/doc/safe-shell/)
set -euv -o pipefail

# check that only *.sh, *.py and a few specific files have the executable bit
# set - any other committed file with mode 100755 is considered a mistake
(git ls-files --stage | awk '$1 == "100755" { print $4 }' | \
  grep -vE '\.(sh|py)$|^dist/appimage/AppRun$') && exit 1

# check if all files have Unix line endings (except 3rd-party libs)
(git grep -Il $'\r' -- ':/' ':!/libs/polyclipping/') && exit 1

# check if no file contains trailing spaces (with some exceptions)
(git grep -Il ' $' -- ':/' ':!/share/librepcb/licenses/' ':!*.dia') && exit 1

# check if no file contains tabulators (with some exceptions)
(git grep -Il $'\t' -- ':/' ':!/LICENSES/' ':!/libs/polyclipping/' ':!/share/librepcb/licenses/' ':!*.ui') && exit 1

# check if no font weights are used that we don't bundle font files for
(git grep -E 'font-weight: [0-9]+' -- '*.slint' | grep -vE 'font-weight: (300|400|700)') && exit 1

# check that std-widgets.slint is not used anywhere as we have our own elements
(git grep -Il 'std-widgets.slint' -- '*.slint') && exit 1

# run all checks from format_code.sh
#  - checks c++ files with clang-format
#  - checks *.rs files with rustfmt
#  - checks *.slint files with slint-lsp
#  - checks *.ui files with python
#  - checks *.qrc files with xmlsort
#  - checks cmake files with cmake-format
#  - checks .reuse/dep5 with debian-copyright-sorter
(./dev/format_code.sh --all --check) || exit 1

# lint rust crates
cargo_toml_files=$(git ls-files -- '**Cargo.toml')
for f in $cargo_toml_files; do
  (cargo clippy --manifest-path="$f" --features="fail-on-warnings,ffi") || exit 1
done

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

# validate AppStream files
appstream-util validate share/metainfo/org.librepcb.LibrePCB.metainfo.xml
