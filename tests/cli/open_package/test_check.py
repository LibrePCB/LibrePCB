#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

import params
from helpers import nofmt

"""
Test command "open-package --check"
"""


def test_no_warnings(cli):
    """
    Test checking a specific package that has no warnings.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    pkg_path = cli.abspath(
        os.path.join(library.dir, "pkg", "2d00d07c-bfc1-4a96-a1cb-195c5ff93db9")
    )
    code, stdout, stderr = cli.run("open-package", "--check", pkg_path)

    assert stderr == ""
    assert stdout == nofmt(f"""\
Open package '{pkg_path}'...
Run checks...
  Approved messages: 0
  Non-approved messages: 0
SUCCESS
""")
    assert code == 0


def test_approved_warnings(cli):
    """
    Test checking a specific package that has only approved warnings.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    pkg_path = cli.abspath(
        os.path.join(library.dir, "pkg", "2d00d07c-bfc1-4a96-a1cb-195c5ff93db9")
    )

    # Add a warning and its corresponding approval
    lp_path = os.path.join(pkg_path, "package.lp")
    with open(lp_path, "r") as f:
        content = f.read()
    content = content.replace("RESC2012", "resc2012")
    content = content.replace("\n)\n", "\n (approved name_not_title_case)\n)\n")
    with open(lp_path, "w") as f:
        f.write(content)

    code, stdout, stderr = cli.run("open-package", "--check", pkg_path)

    assert stderr == ""
    assert stdout == nofmt(f"""\
Open package '{pkg_path}'...
Run checks...
  Approved messages: 1
  Non-approved messages: 0
SUCCESS
""")
    assert code == 0


def test_nonapproved_warnings(cli):
    """
    Test checking a specific package that has non-approved warnings.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    pkg_path = cli.abspath(
        os.path.join(library.dir, "pkg", "2d00d07c-bfc1-4a96-a1cb-195c5ff93db9")
    )

    # Add warnings
    lp_path = os.path.join(pkg_path, "package.lp")
    with open(lp_path, "r") as f:
        content = f.read()
    content = content.replace("RESC2012", "resc2012")
    content = content.replace("Some Author", "")
    with open(lp_path, "w") as f:
        f.write(content)

    code, stdout, stderr = cli.run("open-package", "--check", pkg_path)

    assert stderr == nofmt("""\
    - [WARNING] Author not set
    - [HINT] Name not title case: 'resc2012 (0805)'
""")
    assert stdout == nofmt(f"""\
Open package '{pkg_path}'...
Run checks...
  Approved messages: 0
  Non-approved messages: 2
Finished with errors!
""")
    assert code == 1


def test_invalid_package(cli):
    """
    Test checking with an invalid package path.
    """
    pkg_path = cli.abspath("nonexistent")
    check_path = os.path.join(pkg_path, ".librepcb-pkg")
    code, stdout, stderr = cli.run("open-package", "--check", pkg_path)
    assert stderr == nofmt(f"""\
ERROR: File '{check_path}' does not exist.
""")
    assert stdout == nofmt(f"""\
Open package '{pkg_path}'...
Finished with errors!
""")
    assert code == 1
