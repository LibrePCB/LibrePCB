#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params

"""
Test command "open-library --save"
"""


def test_save(cli):
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    # append some zeros to the library file and a symbol file
    paths = [
        cli.abspath(library.dir + '/library.lp'),
        cli.abspath(library.dir + '/sym/9b75d0ce-ac4e-4a52-a88a-8777f66d3241/symbol.lp'),
    ]
    for path in paths:
        with open(path, 'ab') as f:
            f.write(b'\0\0')
    original_filesizes = [os.path.getsize(path) for path in paths]
    # save library (must remove the appended zeros)
    code, stdout, stderr = cli.run('open-library', '--all', '--save',
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
    filesizes = [os.path.getsize(path) for path in paths]
    assert all([s[0] == (s[1] - 2) for s in zip(filesizes, original_filesizes)])
