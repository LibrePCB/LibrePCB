#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
from helpers import nofmt

"""
Test command "open-library --strict"
"""


def test_valid_lp(cli):
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    code, stdout, stderr = cli.run("open-library", "--all", "--strict", library.dir)
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


def test_invalid_lp(cli):
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    # append some spaces to the library file and a symbol file
    paths = [
        library.dir + "/library.lp",
        library.dir + "/sym/9b75d0ce-ac4e-4a52-a88a-8777f66d3241/symbol.lp",
    ]
    for path in paths:
        with open(cli.abspath(path), "ab") as f:
            f.write(b"  ")
    # open library
    code, stdout, stderr = cli.run("open-library", "--all", "--strict", library.dir)
    assert stderr == nofmt(f"""\
  - Populated Library (a7cb5051-9f37-4500-be38-33eade6e621e):
    - Non-canonical file: '{paths[0].replace("/", os.sep)}'
  - Diode (9b75d0ce-ac4e-4a52-a88a-8777f66d3241):
    - Non-canonical file: '{paths[1].replace("/", os.sep)}'
""")
    assert stdout == nofmt(f"""\
Open library 'Populated Library.lplib'...
Process {library.cmpcat} component categories...
Process {library.pkgcat} package categories...
Process {library.sym} symbols...
Process {library.pkg} packages...
Process {library.cmp} components...
Process {library.dev} devices...
Finished with errors!
""")
    assert code == 1
