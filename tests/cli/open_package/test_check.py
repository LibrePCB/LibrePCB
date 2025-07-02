#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

import params
from helpers import nofmt

"""
Test command "open-package --check"
"""


def test_check_specific_package_with_warnings(cli):
    """Test checking a specific package that has warnings."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    # Check the TO220AB package
    pkg_path = cli.abspath(
        os.path.join(library.dir, "pkg", "0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1")
    )
    code, stdout, stderr = cli.run("open-package", "--check", pkg_path)

    # Should complete successfully even with warnings
    assert stdout == nofmt(f"""\
Open package '{pkg_path}'...
Check '{pkg_path}' for non-approved messages...
  Approved messages: 0
  Non-approved messages: 23
Finished with errors!
""")

    assert stderr == nofmt("""\
  - [WARNING] Clearance of pad '1' in 'Horizontal' to legend
  - [WARNING] Clearance of pad '1' in 'Vertical' to legend
  - [WARNING] Clearance of pad '2' in 'Horizontal' to legend
  - [WARNING] Clearance of pad '2' in 'Vertical' to legend
  - [WARNING] Clearance of pad '3' in 'Horizontal' to legend
  - [WARNING] Clearance of pad '3' in 'Vertical' to legend
  - [WARNING] Minimum width of 'Top Legend' in 'Horizontal'
  - [WARNING] Minimum width of 'Top Legend' in 'Horizontal'
  - [WARNING] Minimum width of 'Top Legend' in 'Horizontal'
  - [WARNING] Minimum width of 'Top Legend' in 'Horizontal'
  - [WARNING] Missing courtyard in footprint 'Vertical'
  - [WARNING] Missing outline in footprint 'Horizontal'
  - [WARNING] Missing outline in footprint 'Vertical'
  - [HINT] No 3D model defined for 'Horizontal'
  - [HINT] Non-recommended assembly type
  - [HINT] Origin of 'Horizontal' not in center
  - [HINT] Origin of 'Vertical' not in center
  - [HINT] Unspecified function of pad '1' in 'Horizontal'
  - [HINT] Unspecified function of pad '1' in 'Vertical'
  - [HINT] Unspecified function of pad '2' in 'Horizontal'
  - [HINT] Unspecified function of pad '2' in 'Vertical'
  - [HINT] Unspecified function of pad '3' in 'Horizontal'
  - [HINT] Unspecified function of pad '3' in 'Vertical'
""")

    assert code == 1


def test_check_invalid_path(cli):
    """Test checking with an invalid package path."""
    invalid_path = cli.abspath("nonexistent")
    code, stdout, stderr = cli.run("open-package", "--check", invalid_path)
    check_path = os.path.join(invalid_path, ".librepcb-pkg")
    # Should fail with appropriate error
    assert code == 1
    assert stderr == f"ERROR: File '{check_path}' does not exist.\n"
