#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
from helpers import nofmt

"""
Test command "open-library --save"
"""


def test_save(cli):
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    # append some spaces to the library file and a symbol file
    paths = [
        cli.abspath(library.dir + "/library.lp"),
        cli.abspath(
            library.dir + "/sym/9b75d0ce-ac4e-4a52-a88a-8777f66d3241/symbol.lp"
        ),
    ]
    for path in paths:
        with open(path, "ab") as f:
            f.write(b"  ")
    original_filesizes = [os.path.getsize(path) for path in paths]
    # save library (must remove the appended zeros)
    code, stdout, stderr = cli.run("open-library", "--all", "--save", library.dir)
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open library '{library.dir}'...
Process {library.cmpcat} component categories...
Process {library.pkgcat} package categories...
Process {library.sym} symbols...
Process {library.pkg} packages...
Process {library.cmp} components...
Process {library.dev} devices...
SUCCESS
""")
    assert code == 0
    filesizes = [os.path.getsize(path) for path in paths]
    assert all([s[0] == (s[1] - 2) for s in zip(filesizes, original_filesizes)])
