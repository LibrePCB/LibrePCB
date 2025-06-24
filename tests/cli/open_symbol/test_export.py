#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

import params
import pytest
from helpers import _clean, nofmt

"""
Test command "open-symbol --export"
"""


@pytest.mark.parametrize(
    "sym_uuid",
    [
        "f00ab942-6980-442b-86a8-51b92de5704d",  # A4 Frame symbol
    ],
)
def test_if_unknown_file_extension_fails(cli, sym_uuid):
    """
    Test that exporting with an unknown file extension fails.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    sym_path = os.path.join(library.dir, "sym", sym_uuid)

    export_path = "symbol.foo"
    code, stdout, stderr = cli.run("open-symbol", "--export", export_path, sym_path)

    assert code == 1
    assert (
        _clean(stderr)
        == "  ERROR: Unknown extension 'foo'. Supported extensions: pdf, svg, ***\n"
    )
    assert stdout == nofmt(f"""\
Open symbol '{sym_path}'...
Export symbol to '{export_path}'...
Finished with errors!
""")
    assert not os.path.exists(cli.abspath(export_path))


@pytest.mark.parametrize(
    "sym_uuid",
    [
        "f00ab942-6980-442b-86a8-51b92de5704d",
    ],
)
def test_export_symbol_to_png_relative(cli, sym_uuid):
    """
    Test exporting a symbol to PNG format with a relative path.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    sym_path = os.path.join(library.dir, "sym", sym_uuid)

    export_path = "symbols/test_symbol.png"
    code, stdout, stderr = cli.run("open-symbol", "--export", export_path, sym_path)

    assert code == 0
    assert stderr == ""

    # Check that the symbol was exported
    symbol_file = cli.abspath(export_path)
    assert os.path.exists(symbol_file)
    assert os.path.getsize(symbol_file) > 0


@pytest.mark.parametrize(
    "sym_uuid",
    [
        "f00ab942-6980-442b-86a8-51b92de5704d",
    ],
)
def test_export_symbol_to_svg_absolute(cli, sym_uuid):
    """
    Test exporting a symbol to SVG format with an absolute path.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    sym_path = os.path.join(library.dir, "sym", sym_uuid)

    export_dir = cli.abspath("svg_exports")
    export_path = os.path.join(export_dir, "symbol with spaces.svg")
    code, stdout, stderr = cli.run("open-symbol", "--export", export_path, sym_path)

    assert code == 0
    assert stderr == ""

    # Check that the symbol was exported
    assert os.path.exists(export_path)
    assert os.path.getsize(export_path) > 0


def test_export_symbol_with_substitutions(cli):
    """Test exporting a symbol using filename substitutions."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    sym_path = os.path.join(library.dir, "sym", "f00ab942-6980-442b-86a8-51b92de5704d")

    # Use substitution patterns
    export_pattern = "{{SYMBOL}}.png"
    code, stdout, stderr = cli.run("open-symbol", "--export", export_pattern, sym_path)

    # Should succeed
    assert code == 0
    assert stderr == ""

    # Check that a file was created with substitutions applied
    files = os.listdir(cli.abspath("."))

    # Should have created a file
    png_files = [f for f in files if f.endswith(".png")]
    assert len(png_files) > 0

    # The file should have the symbol name in it
    assert any("A4" in f for f in png_files)


def test_if_output_directories_are_created(cli):
    """
    Test if output directories are created automatically.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    sym_uuid = "f00ab942-6980-442b-86a8-51b92de5704d"
    sym_path = os.path.join(library.dir, "sym", sym_uuid)

    export_dir = cli.abspath("nonexistent/nested/dir")
    export_path = os.path.join(export_dir, "symbol.pdf")
    assert not os.path.exists(export_dir)

    code, stdout, stderr = cli.run("open-symbol", "--export", export_path, sym_path)
    assert code == 0
    assert stderr == ""
    assert os.path.isdir(export_dir)
    assert os.path.exists(export_path)


def test_export_multiple_formats(cli):
    """Test exporting a symbol to different formats."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    sym_path = os.path.join(library.dir, "sym", "f00ab942-6980-442b-86a8-51b92de5704d")

    formats = [".png", ".svg", ".pdf"]

    for fmt in formats:
        export_path = f"symbol{fmt}"
        code, stdout, stderr = cli.run("open-symbol", "--export", export_path, sym_path)

        # Should succeed for supported formats
        assert code == 0
        assert os.path.exists(cli.abspath(export_path))
        assert os.path.getsize(cli.abspath(export_path)) > 0


def test_export_invalid_symbol_path(cli):
    """
    Test exporting with an invalid symbol path.
    """
    export_path = "symbol.png"
    invalid_sym_path = cli.abspath("nonexistent")
    code, stdout, stderr = cli.run(
        "open-symbol", "--export", export_path, invalid_sym_path
    )

    check_path = os.path.join(invalid_sym_path, ".librepcb-sym")
    # Should fail with appropriate error
    assert code != 0
    assert stderr == f"ERROR: File '{check_path}' does not exist.\n"
    assert stdout == f"Open symbol '{invalid_sym_path}'...\nFinished with errors!\n"
    assert not os.path.exists(cli.abspath(export_path))
