#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

import params
import pytest
from helpers import nofmt

"""
Test command "open-package --export"
"""


@pytest.mark.parametrize(
    "pkg_uuid",
    [
        "0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1",  # A package with multiple footprints
    ],
)
def test_if_unknown_file_extension_fails(cli, pkg_uuid):
    """
    Test that exporting with an unknown file extension fails.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    pkg_path = os.path.join(library.dir, "pkg", pkg_uuid)

    export_path = "package.foo"
    code, stdout, stderr = cli.run("open-package", "--export", export_path, pkg_path)
    assert code == 1
    # Get the list of supported extensions for the error message
    # This is a bit hacky but matches what the CLI shows
    assert "ERROR: Unknown extension 'foo'" in stderr
    assert "Supported extensions:" in stderr
    assert stdout == nofmt(f"""\
Open package '{pkg_path}'...
Export package to '{export_path}'...
Finished with errors!
""")
    assert not os.path.exists(cli.abspath(export_path))


@pytest.mark.parametrize(
    "pkg_uuid",
    [
        "0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1",
    ],
)
def test_export_package_to_png_relative(cli, pkg_uuid):
    """
    Test exporting a package to PNG format with a relative path.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    pkg_path = os.path.join(library.dir, "pkg", pkg_uuid)

    # Use substitution patterns to avoid conflicts
    export_path = "footprints/{{FOOTPRINT}}.png"
    code, stdout, stderr = cli.run("open-package", "--export", export_path, pkg_path)

    assert code == 0
    assert "SUCCESS" in stdout
    assert stderr == ""

    # Check that all footprints were exported
    footprint_dir = cli.abspath("footprints")
    assert os.path.isdir(footprint_dir)
    exported_files = os.listdir(footprint_dir)
    # This package has 2 footprints, so we expect 2 PNGs
    assert len(exported_files) == 2
    for f in exported_files:
        assert f.endswith(".png")
        assert os.path.getsize(os.path.join(footprint_dir, f)) > 0


@pytest.mark.parametrize(
    "pkg_uuid",
    [
        "0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1",
    ],
)
def test_export_package_to_svg_absolute(cli, pkg_uuid):
    """
    Test exporting a package to SVG format with an absolute path.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    pkg_path = os.path.join(library.dir, "pkg", pkg_uuid)

    # Use substitution patterns to avoid conflicts
    export_dir = cli.abspath("svg_exports")
    export_path = os.path.join(export_dir, "{{FOOTPRINT}} with spaces.svg")
    code, stdout, stderr = cli.run("open-package", "--export", export_path, pkg_path)

    assert code == 0
    assert "SUCCESS" in stdout
    assert stderr == ""

    # Check that all footprints were exported
    assert os.path.isdir(export_dir)
    exported_files = os.listdir(export_dir)
    assert len(exported_files) == 2
    for f in exported_files:
        assert f.endswith(".svg")
        assert " with spaces" in f
        assert os.path.getsize(os.path.join(export_dir, f)) > 0


def test_if_output_directories_are_created(cli):
    """
    Test if output directories are created automatically.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    pkg_uuid = "0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1"
    pkg_path = os.path.join(library.dir, "pkg", pkg_uuid)

    export_dir = cli.abspath("nonexistent/nested/dir")
    export_path = os.path.join(export_dir, "{{FOOTPRINT}}.pdf")
    assert not os.path.exists(export_dir)

    code, stdout, stderr = cli.run("open-package", "--export", export_path, pkg_path)

    assert code == 0
    assert "SUCCESS" in stdout
    assert stderr == ""
    assert os.path.isdir(export_dir)

    exported_files = os.listdir(export_dir)
    assert len(exported_files) == 2


def test_export_with_conflicting_filenames_fails(cli):
    """
    Test that exporting fails if multiple footprints would be written to the same file.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    # This package has multiple footprints
    pkg_uuid = "0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1"
    pkg_path = os.path.join(library.dir, "pkg", pkg_uuid)

    export_path = "package.png"
    code, stdout, stderr = cli.run("open-package", "--export", export_path, pkg_path)

    assert code == 1
    # The CLI shows a relative path in the error message
    assert stderr == nofmt(f"""\
ERROR: The file '{export_path}' was written multiple times!
NOTE: To avoid writing files multiple times, make sure to pass unique filepaths \
to all export functions. For package output files, you could add a placeholder \
like '{{{{FOOTPRINT}}}}' to the path.
""")
    assert "Finished with errors!" in stdout
    # The file should be created (the last footprint's export succeeds)
    abs_export_path = cli.abspath(export_path)
    assert os.path.exists(abs_export_path)


def test_export_invalid_package_path(cli):
    """
    Test exporting with an invalid package path.
    """
    export_path = "package.png"
    invalid_pkg_path = "/nonexistent/package"
    code, stdout, stderr = cli.run(
        "open-package", "--export", export_path, invalid_pkg_path
    )

    assert code == 1
    assert "does not exist" in stderr
    assert "Finished with errors!" in stdout
    assert not os.path.exists(cli.abspath(export_path))
