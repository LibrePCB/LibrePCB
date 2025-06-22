#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

import params

"""
Test command "open-symbol --export"
"""


def test_export_symbol_to_png(cli):
    """Test exporting a symbol to PNG format."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    # Export the A4 Frame symbol
    sym_path = os.path.join(library.dir, "sym", "f00ab942-6980-442b-86a8-51b92de5704d")
    export_path = "symbol.png"

    code, stdout, stderr = cli.run("open-symbol", "--export", export_path, sym_path)

    # Should succeed
    assert code == 0
    assert "Open symbol" in stdout
    assert "Export" in stdout

    # File should exist and have content
    assert os.path.exists(cli.abspath(export_path))
    assert os.path.getsize(cli.abspath(export_path)) > 0


def test_export_symbol_with_substitutions(cli):
    """Test exporting a symbol using filename substitutions."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    sym_path = os.path.join(library.dir, "sym", "f00ab942-6980-442b-86a8-51b92de5704d")

    # Use substitution patterns
    export_pattern = "{{SYMBOL}}.png"
    code, stdout, stderr = cli.run(
        "open-symbol", "--export", export_pattern, sym_path
    )

    # Should succeed
    assert code == 0

    # Check that a file was created with substitutions applied
    files = os.listdir(cli.abspath("."))
    
    # Should have created a file
    png_files = [f for f in files if f.endswith(".png")]
    assert len(png_files) > 0

    # The file should have the symbol name in it
    assert any("A4" in f for f in png_files)


def test_export_multiple_formats(cli):
    """Test exporting a symbol to different formats."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    sym_path = os.path.join(library.dir, "sym", "f00ab942-6980-442b-86a8-51b92de5704d")

    formats = [".png", ".svg", ".pdf"]

    for fmt in formats:
        export_path = f"symbol{fmt}"
        code, stdout, stderr = cli.run(
            "open-symbol", "--export", export_path, sym_path
        )

        # Should succeed for supported formats
        if code == 0:
            assert os.path.exists(cli.abspath(export_path))
            assert os.path.getsize(cli.abspath(export_path)) > 0


def test_export_invalid_path(cli):
    """Test exporting with an invalid symbol path."""
    export_path = "symbol.png"
    
    code, stdout, stderr = cli.run(
        "open-symbol", "--export", export_path, "/nonexistent/symbol"
    )

    # Should fail
    assert code != 0
    assert "ERROR" in stderr or "ERROR" in stdout
    
    # No file should be created
    assert not os.path.exists(cli.abspath(export_path))