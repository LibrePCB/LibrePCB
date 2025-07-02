#!/usr/bin/env python
# -*- coding: utf-8 -*-


import os
from pathlib import Path

import params
from helpers import nofmt

"""
Test command "open-symbol --check"
"""


def test_no_warnings(cli):
    """
    Test checking a specific symbol that has no warnings.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    sym_path = cli.abspath(
        os.path.join(library.dir, "sym", "9b75d0ce-ac4e-4a52-a88a-8777f66d3241")
    )
    code, stdout, stderr = cli.run("open-symbol", "--check", sym_path)

    assert stdout == nofmt(f"""\
Open symbol '{sym_path}'...
Run checks...
  Approved messages: 0
  Non-approved messages: 0
SUCCESS
""")
    assert stderr == ""
    assert code == 0


def test_approved_warnings(cli):
    """
    Test checking a specific symbol that has only approved warnings.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    sym_path = cli.abspath(
        os.path.join(library.dir, "sym", "9b75d0ce-ac4e-4a52-a88a-8777f66d3241")
    )

    # Add a warning and its corresponding approval
    lp_path = os.path.join(sym_path, "symbol.lp")
    with open(lp_path, "r") as f:
        content = f.read()
    content = content.replace("Diode", "diode")
    content = content.replace("\n)\n", "\n (approved name_not_title_case)\n)\n")
    with open(lp_path, "w") as f:
        f.write(content)

    code, stdout, stderr = cli.run("open-symbol", "--check", sym_path)

    assert stderr == ""
    assert stdout == nofmt(f"""\
Open symbol '{sym_path}'...
Run checks...
  Approved messages: 1
  Non-approved messages: 0
SUCCESS
""")
    assert code == 0


def test_nonapproved_warnings(cli):
    """
    Test checking a specific symbol that has non-approved warnings.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    sym_path = cli.abspath(
        os.path.join(library.dir, "sym", "9b75d0ce-ac4e-4a52-a88a-8777f66d3241")
    )

    # Add warnings
    lp_path = os.path.join(sym_path, "symbol.lp")
    with open(lp_path, "r") as f:
        content = f.read()
    content = content.replace("Diode", "diode")
    content = content.replace("LibrePCB", "")
    with open(lp_path, "w") as f:
        f.write(content)

    code, stdout, stderr = cli.run("open-symbol", "--check", sym_path)

    assert stderr == nofmt("""\
    - [WARNING] Author not set
    - [HINT] Name not title case: 'diode'
""")
    assert stdout == nofmt(f"""\
Open symbol '{sym_path}'...
Run checks...
  Approved messages: 0
  Non-approved messages: 2
Finished with errors!
""")
    assert code == 1


def test_invalid_symbol(cli):
    """
    Test checking with an invalid symbol path.
    """
    sym_path = cli.abspath("nonexistent")
    check_path = Path(sym_path, ".librepcb-sym")
    code, stdout, stderr = cli.run("open-symbol", "--check", sym_path)
    assert stderr == nofmt(f"""\
ERROR: File '{check_path}' does not exist.
""")
    assert stdout == nofmt(f"""\
Open symbol '{sym_path}'...
Finished with errors!
""")
    assert code == 1
