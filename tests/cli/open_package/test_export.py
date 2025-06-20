#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import tempfile

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

    with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
        export_path = tmp.name

    try:
        code, stdout, stderr = cli.run(
            "open-package", "--export", export_path, pkg_path
        )

        # Should succeed
        assert code == 0
        assert "Open package" in stdout
        assert "Export" in stdout

        # File should exist and have content
        assert os.path.exists(export_path)
        assert os.path.getsize(export_path) > 0
    finally:
        if os.path.exists(export_path):
            os.unlink(export_path)


def test_export_package_with_substitutions(cli):
    """Test exporting a package using filename substitutions."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    pkg_path = os.path.join(library.dir, "pkg", "0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1")

    with tempfile.TemporaryDirectory() as tmpdir:
        # Use substitution patterns
        export_pattern = os.path.join(tmpdir, "{{PACKAGE_NAME}}_{{FOOTPRINT_NAME}}.png")
        code, stdout, stderr = cli.run(
            "open-package", "--export", export_pattern, pkg_path
        )

        # Should succeed
        assert code == 0

        # Check that a file was created with substitutions applied
        files = os.listdir(tmpdir)
        assert len(files) > 0

        # The file should have the package name in it
        assert any("TO220" in f for f in files)


def test_export_multiple_formats(cli):
    """Test exporting a package to different formats."""
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)

    pkg_path = os.path.join(library.dir, "pkg", "0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1")

    formats = [".png", ".svg", ".pdf"]

    with tempfile.TemporaryDirectory() as tmpdir:
        for fmt in formats:
            export_path = os.path.join(tmpdir, f"package{fmt}")
            code, stdout, stderr = cli.run(
                "open-package", "--export", export_path, pkg_path
            )

            # Should succeed for supported formats
            if code == 0:
                assert os.path.exists(export_path)
                assert os.path.getsize(export_path) > 0


def test_export_invalid_path(cli):
    """Test exporting with an invalid package path."""
    with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
        export_path = tmp.name

    try:
        code, stdout, stderr = cli.run(
            "open-package", "--export", export_path, "/nonexistent/package"
        )

        # Should fail
        assert code != 0
        assert "ERROR" in stderr or "ERROR" in stdout
    finally:
        if os.path.exists(export_path):
            os.unlink(export_path)