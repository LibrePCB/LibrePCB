#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

import params

"""
Test command "open-package --check"
"""


def test_check_all_packages(cli):
    """Test checking all packages in a library."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    package_dirs = os.listdir(cli.abspath(os.path.join(library.dir, "pkg")))

    # Test each package individually
    for pkg_uuid in package_dirs:
        pkg_path = os.path.join(library.dir, "pkg", pkg_uuid)
        code, stdout, stderr = cli.run("open-package", "--check", pkg_path)

        # Each package should complete without critical errors
        # Some may have warnings/hints which is okay
        assert "Finished" in stdout or "SUCCESS" in stdout
        assert pkg_uuid in stdout


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
    assert "Open package" in stdout
    assert "Finished" in stdout
    assert "WARNING" in stderr.upper()
    assert code != 0


def test_check_invalid_path(cli):
    """Test checking with an invalid package path."""
    code, stdout, stderr = cli.run(
        "open-package", "--check", "/tmp/nonexistent/package"
    )

    # Should fail with appropriate error
    assert code != 0
    assert "ERROR" in stderr or "ERROR" in stdout


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
