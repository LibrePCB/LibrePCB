#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

import params
from helpers import nofmt, strip_image_file_extensions

"""
Test command "open-symbol --export"
"""


def test_if_unknown_file_extension_fails(cli):
    """
    Test that exporting with an unknown file extension fails.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    sym_path = os.path.join(library.dir, "sym", "f00ab942-6980-442b-86a8-51b92de5704d")
    export_path = "symbol.foo"
    code, stdout, stderr = cli.run("open-symbol", "--export", export_path, sym_path)

    assert strip_image_file_extensions(stderr) == nofmt(f"""\
  ERROR: Failed to export image '{cli.abspath(export_path)}' due to unknown \
file extension. Supported extensions: pdf, svg, ***
""")
    assert stdout == nofmt(f"""\
Open symbol '{sym_path}'...
Export symbol to '{export_path}'...
Finished with errors!
""")
    assert code == 1
    assert not os.path.exists(cli.abspath(export_path))


def test_export_symbol_to_png_relative(cli):
    """
    Test exporting a symbol to PNG format with a relative path.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    sym_path = os.path.join(library.dir, "sym", "f00ab942-6980-442b-86a8-51b92de5704d")

    export_path = "symbols/test_symbol.png"
    code, stdout, stderr = cli.run("open-symbol", "--export", export_path, sym_path)

    assert stderr == ""
    assert stdout == nofmt(f"""\
Open symbol '{sym_path}'...
Export symbol to '{export_path}'...
  => 'symbols//test_symbol.png'
SUCCESS
""").replace("//", os.sep)
    assert code == 0

    # Check that the symbol was exported
    symbol_file = cli.abspath(export_path)
    assert os.path.exists(symbol_file)
    assert os.path.getsize(symbol_file) > 0


def test_export_symbol_to_svg_absolute(cli):
    """
    Test exporting a symbol to SVG format with an absolute path.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    sym_path = os.path.join(library.dir, "sym", "f00ab942-6980-442b-86a8-51b92de5704d")

    export_dir = cli.abspath("svg_exports")
    export_path = os.path.join(export_dir, "symbol with spaces.svg")
    code, stdout, stderr = cli.run("open-symbol", "--export", export_path, sym_path)

    assert stderr == ""
    assert stdout == nofmt(f"""\
Open symbol '{sym_path}'...
Export symbol to '{export_path}'...
  => '{export_dir}//symbol with spaces.svg'
SUCCESS
""").replace("//", os.sep)
    assert code == 0

    # Check that the symbol was exported
    assert os.path.exists(export_path)
    assert os.path.getsize(export_path) > 0


def test_pdf_with_substitutions(cli):
    """
    Test exporting a symbol to PDF using filename substitutions.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    sym_path = os.path.join(library.dir, "sym", "f00ab942-6980-442b-86a8-51b92de5704d")

    # Use substitution patterns
    export_dir = cli.abspath("out")
    export_pattern = "out/{{SYMBOL}}.pdf"
    code, stdout, stderr = cli.run("open-symbol", "--export", export_pattern, sym_path)

    # Should succeed
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open symbol '{sym_path}'...
Export symbol to '{export_pattern}'...
  => 'out//A4_Frame_Landscape.pdf'
SUCCESS
""").replace("//", os.sep)
    assert code == 0
    assert set(os.listdir(export_dir)) == set(["A4_Frame_Landscape.pdf"])


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

    assert stderr == ""
    assert stdout == nofmt(f"""\
Open symbol '{sym_path}'...
Export symbol to '{export_path}'...
  => '{export_path}'
SUCCESS
""")
    assert code == 0
    assert os.path.isdir(export_dir)
    assert set(os.listdir(export_dir)) == set(["symbol.pdf"])


def test_invalid_symbol(cli):
    """
    Test exporting with an invalid symbol path.
    """
    export_path = "symbol.png"
    sym_path = cli.abspath("nonexistent")
    code, stdout, stderr = cli.run("open-symbol", "--export", export_path, sym_path)
    check_path = os.path.join(sym_path, ".librepcb-sym")

    assert stderr == nofmt(f"""\
ERROR: File '{check_path}' does not exist.
""")
    assert stdout == nofmt(f"""\
Open symbol '{sym_path}'...
Finished with errors!
""")
    assert code == 1
    assert not os.path.exists(cli.abspath(export_path))
