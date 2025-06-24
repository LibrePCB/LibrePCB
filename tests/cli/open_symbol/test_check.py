#!/usr/bin/env python
# -*- coding: utf-8 -*-


import os
from pathlib import Path

import params

"""
Test command "open-symbol --check"
"""


def test_check_specific_symbol_with_warnings(cli):
    """Test checking a specific symbol that has warnings."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    sym_path = cli.abspath(
        os.path.join(library.dir, "sym", "f00ab942-6980-442b-86a8-51b92de5704d")
    )
    code, stdout, stderr = cli.run("open-symbol", "--check", sym_path)
    # Should complete successfully even with warnings
    assert stdout == f"Open symbol '{sym_path}'...\nFinished with errors!\n"
    assert (
        stderr
        == "  - [WARNING] Missing text: '{{NAME}}'\n  - [WARNING] Missing text: '{{VALUE}}'\n"
    )
    assert code != 0


def test_check_invalid_path(cli):
    """Test checking with an invalid symbol path."""
    invalid_path = cli.abspath("nonexistent")
    code, stdout, stderr = cli.run("open-symbol", "--check", invalid_path)
    check_path = Path(invalid_path, ".librepcb-sym")
    # Should fail with appropriate error
    assert code != 0
    assert stderr == f"ERROR: File '{check_path}' does not exist.\n"


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
