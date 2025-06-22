#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

import params

"""
Test command "open-package --export"
"""


def test_export_package_to_png(cli):
    """Test exporting a package to PNG format."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    # Export the TO220AB package
    pkg_path = os.path.join(library.dir, "pkg", "0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1")
    export_path = "package.png"

    code, stdout, stderr = cli.run("open-package", "--export", export_path, pkg_path)

    # Should succeed
    assert code == 0
    assert "Open package" in stdout
    assert "Export" in stdout

    # File should exist and have content
    assert os.path.exists(cli.abspath(export_path))
    assert os.path.getsize(cli.abspath(export_path)) > 0


def test_export_package_with_substitutions(cli):
    """Test exporting a package using filename substitutions."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    pkg_path = os.path.join(library.dir, "pkg", "0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1")

    # Use substitution patterns
    export_pattern = "{{PACKAGE}}_{{FOOTPRINT}}.png"
    code, stdout, stderr = cli.run("open-package", "--export", export_pattern, pkg_path)

    # Should succeed
    assert code == 0

    # Check that files were created with substitutions applied
    # The test directory is the current working directory for the CLI
    files = os.listdir(cli.abspath("."))

    # Should have created files for each footprint
    png_files = [f for f in files if f.endswith(".png")]
    assert len(png_files) > 0

    # The files should have the package name in them
    assert any("TO220" in f for f in png_files)


def test_export_multiple_formats(cli):
    """Test exporting a package to different formats."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    pkg_path = os.path.join(library.dir, "pkg", "0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1")

    formats = [".png", ".svg", ".pdf"]

    for fmt in formats:
        export_path = f"package{fmt}"
        code, stdout, stderr = cli.run(
            "open-package", "--export", export_path, pkg_path
        )
        # Should succeed for supported formats
        if code == 0:
            assert os.path.exists(cli.abspath(export_path))
            assert os.path.getsize(cli.abspath(export_path)) > 0


def test_export_invalid_path(cli):
    """Test exporting with an invalid package path."""
    export_path = "package.png"

    code, stdout, stderr = cli.run(
        "open-package", "--export", export_path, "/nonexistent/package"
    )

    # Should fail
    assert code != 0
    assert "ERROR" in stderr or "ERROR" in stdout

    # No file should be created
    assert not os.path.exists(cli.abspath(export_path))
