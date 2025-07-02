#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

import params
from helpers import nofmt, strip_image_file_extensions

"""
Test command "open-package --export"
"""


def test_if_unknown_file_extension_fails(cli):
    """
    Test that exporting with an unknown file extension fails.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    pkg_path = os.path.join(library.dir, "pkg", "0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1")
    export_path = "package.foo"
    code, stdout, stderr = cli.run("open-package", "--export", export_path, pkg_path)

    # We get the error twice - once for each failing footprint
    assert strip_image_file_extensions(stderr) == nofmt(f"""\
  ERROR: Failed to export image '{cli.abspath(export_path)}' due to unknown \
file extension. Supported extensions: pdf, svg, ***
  ERROR: Failed to export image '{cli.abspath(export_path)}' due to unknown \
file extension. Supported extensions: pdf, svg, ***
""")
    assert stdout == nofmt(f"""\
Open package '{pkg_path}'...
Export footprint(s) to '{export_path}'...
Finished with errors!
""")
    assert code == 1
    assert not os.path.exists(cli.abspath(export_path))


def test_export_to_png_relative(cli):
    """
    Test exporting a package to PNG format with a relative path.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    pkg_path = os.path.join(library.dir, "pkg", "0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1")

    # Use substitution patterns to avoid conflicts
    export_path = os.path.join("footprints", "{{FOOTPRINT}}.png")
    code, stdout, stderr = cli.run("open-package", "--export", export_path, pkg_path)

    assert stderr == ""
    assert stdout == nofmt(f"""\
Open package '{pkg_path}'...
Export footprint(s) to '{export_path}'...
  => 'footprints//Vertical.png'
  => 'footprints//Horizontal.png'
SUCCESS
""").replace("//", os.sep)
    assert code == 0

    # Check that all footprints were exported
    footprint_dir = cli.abspath("footprints")
    assert os.path.isdir(footprint_dir)
    assert set(os.listdir(footprint_dir)) == set(["Vertical.png", "Horizontal.png"])


def test_export_to_svg_absolute(cli):
    """
    Test exporting a package to SVG format with an absolute path.
    """
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    pkg_path = os.path.join(library.dir, "pkg", "0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1")

    # Use substitution patterns to avoid conflicts
    export_dir = cli.abspath("svg_exports")
    export_path = os.path.join(export_dir, "{{FOOTPRINT}} with spaces.svg")
    code, stdout, stderr = cli.run("open-package", "--export", export_path, pkg_path)

    assert stderr == ""
    assert stdout == nofmt(f"""\
Open package '{pkg_path}'...
Export footprint(s) to '{export_path}'...
  => '{export_dir}//Vertical with spaces.svg'
  => '{export_dir}//Horizontal with spaces.svg'
SUCCESS
""").replace("//", os.sep)
    assert code == 0

    # Check that all footprints were exported
    assert os.path.isdir(export_dir)
    assert set(os.listdir(export_dir)) == set(
        ["Vertical with spaces.svg", "Horizontal with spaces.svg"]
    )


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

    assert stderr == ""
    assert stdout == nofmt(f"""\
Open package '{pkg_path}'...
Export footprint(s) to '{export_path}'...
  => '{export_dir}//Vertical.pdf'
  => '{export_dir}//Horizontal.pdf'
SUCCESS
""").replace("//", os.sep)
    assert code == 0
    assert os.path.isdir(export_dir)
    assert set(os.listdir(export_dir)) == set(["Vertical.pdf", "Horizontal.pdf"])


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

    # The CLI shows a relative path in the error message
    assert stderr == nofmt(f"""\
ERROR: The file '{export_path}' was written multiple times!
NOTE: To avoid writing files multiple times, make sure to pass unique filepaths \
to all export functions. For footprint output files, you could add a placeholder \
like '{{{{FOOTPRINT}}}}' to the path.
""")
    assert stdout == nofmt(f"""\
Open package '{pkg_path}'...
Export footprint(s) to '{export_path}'...
  => 'package.png'
  => 'package.png'
Finished with errors!
""")
    assert code == 1
    # The file should be created (the last footprint's export succeeds)
    abs_export_path = cli.abspath(export_path)
    assert os.path.exists(abs_export_path)


def test_invalid_package(cli):
    """
    Test exporting with an invalid package path.
    """
    export_path = "package.png"
    pkg_path = cli.abspath("nonexistent")
    code, stdout, stderr = cli.run("open-package", "--export", export_path, pkg_path)
    check_path = os.path.join(pkg_path, ".librepcb-pkg")

    assert stderr == nofmt(f"""\
ERROR: File '{check_path}' does not exist.
""")
    assert stdout == nofmt(f"""\
Open package '{pkg_path}'...
Finished with errors!
""")
    assert code == 1
    assert not os.path.exists(cli.abspath(export_path))
