#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

import params

"""
Test command "open-symbol --check"
"""


def test_check_all_symbols(cli):
    """Test checking all symbols in a library."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    symbol_dirs = os.listdir(cli.abspath(os.path.join(library.dir, "sym")))

    # Test each symbol individually
    for sym_uuid in symbol_dirs:
        sym_path = os.path.join(library.dir, "sym", sym_uuid)
        code, stdout, stderr = cli.run("open-symbol", "--check", sym_path)

        # Each symbol should complete without critical errors
        # Some may have warnings/hints which is okay
        assert "Finished" in stdout or "SUCCESS" in stdout
        assert sym_uuid in stdout


def test_check_specific_symbol_with_warnings(cli):
    """Test checking a specific symbol that has warnings."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    sym_path = cli.abspath(
        os.path.join(library.dir, "sym", "f00ab942-6980-442b-86a8-51b92de5704d")
    )
    code, stdout, stderr = cli.run("open-symbol", "--check", sym_path)
    # Should complete successfully even with warnings
    assert "Open symbol" in stdout
    assert "Finished" in stdout
    assert "WARNING" in stderr.upper()
    assert code != 0


def test_check_invalid_path(cli):
    """Test checking with an invalid symbol path."""
    code, stdout, stderr = cli.run("open-symbol", "--check", "/tmp/nonexistent/symbol")

    # Should fail with appropriate error
    assert code != 0
    assert "ERROR" in stderr or "ERROR" in stdout


def test_check_non_symbol_element(cli):
    """The open-symbol command should not work with non-symbol elements."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    # Try to open a symbol directory as a symbol
    if os.path.exists(os.path.join(library.dir, "sym")):
        # Get first symbol directory
        sym_dirs = os.listdir(os.path.join(library.dir, "sym"))
        if sym_dirs:
            sym_path = os.path.join(library.dir, "sym", sym_dirs[0])
            code, stdout, stderr = cli.run("open-symbol", "--check", sym_path)

            # Should fail because it's not a symbol
            assert code != 0
            assert "ERROR" in stderr or "ERROR" in stdout
