#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params

"""
Test command "open-library --strict"
"""


def test_valid_lp(cli):
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    code, stdout, stderr = cli.run('open-library', '--all', '--strict',
                                   library.dir)
    assert stderr == ''
    assert stdout == \
        "Open library '{library.dir}'...\n" \
        "Process {library.cmpcat} component categories...\n" \
        "Process {library.pkgcat} package categories...\n" \
        "Process {library.sym} symbols...\n" \
        "Process {library.pkg} packages...\n" \
        "Process {library.cmp} components...\n" \
        "Process {library.dev} devices...\n" \
        "SUCCESS\n".format(library=library)
    assert code == 0


def test_invalid_lp(cli):
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    # append some zeros to the library file and a symbol file
    paths = [
        library.dir + '/library.lp',
        library.dir + '/sym/9b75d0ce-ac4e-4a52-a88a-8777f66d3241/symbol.lp',
    ]
    for path in paths:
        with open(cli.abspath(path), 'ab') as f:
            f.write(b'\0\0')
    # open library
    code, stdout, stderr = cli.run('open-library', '--all', '--strict',
                                   library.dir)
    assert stderr == \
        "    - Non-canonical file: '{paths[0]}'\n" \
        "    - Non-canonical file: '{paths[1]}'\n" \
        .format(paths=paths).replace('/', os.sep)
    assert stdout == \
        "Open library 'Populated Library.lplib'...\n" \
        "Process {library.cmpcat} component categories...\n" \
        "Process {library.pkgcat} package categories...\n" \
        "Process {library.sym} symbols...\n" \
        "Process {library.pkg} packages...\n" \
        "Process {library.cmp} components...\n" \
        "Process {library.dev} devices...\n" \
        "Finished with errors!\n".format(library=library)
    assert code == 1
