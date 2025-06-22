#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

import params

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
    assert stdout == f"Open package '{pkg_path}'...\nFinished with errors!\n"
    assert (
        stderr
        == """  - [WARNING] Clearance of pad '1' in 'Horizontal' to legend
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
"""
    )
    assert code != 0


def test_check_invalid_path(cli):
    """Test checking with an invalid package path."""
    invalid_path = "/tmp/nonexistent/package"
    code, stdout, stderr = cli.run("open-package", "--check", invalid_path)

    # Should fail with appropriate error
    assert code != 0
    assert stderr == f"ERROR: File '{invalid_path}/.librepcb-pkg' does not exist.\n"


def test_check_non_package_element(cli):
    """The open-package command should not work with non-package elements."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    # Try to open a symbol directory as a package
    if os.path.exists(os.path.join(library.dir, "sym")):
        # Get first symbol directory
        sym_dirs = os.listdir(os.path.join(library.dir, "sym"))
        if sym_dirs:
            sym_path = os.path.join(library.dir, "sym", sym_dirs[0])
            code, stdout, stderr = cli.run("open-package", "--check", sym_path)

            # Should fail because it's not a package
            assert code != 0
            assert "ERROR" in stderr or "ERROR" in stdout
