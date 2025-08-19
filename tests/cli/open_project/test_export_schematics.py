#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

import params
import pytest
from helpers import nofmt, strip_image_file_extensions

"""
Test command "open-project --export-schematics"
"""


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
    ],
)
def test_if_unknown_file_extension_fails(cli, project):
    cli.suppress_deprecation_warnings = True
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run(
        "open-project", "--export-schematics=foo.bar", project.path
    )
    assert strip_image_file_extensions(stderr) == nofmt(f"""\
  ERROR: Failed to export image '{cli.abspath("foo.bar")}' due to unknown file \
extension. Supported extensions: pdf, svg, ***
""")
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export schematics to 'foo.bar'...
Finished with errors!
""")
    assert code == 1


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
    ],
)
def test_exporting_pdf_with_relative_path(cli, project):
    cli.suppress_deprecation_warnings = True
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    path = cli.abspath("sch.pdf")
    assert not os.path.exists(path)
    code, stdout, stderr = cli.run(
        "open-project", "--export-schematics=sch.pdf", project.path
    )
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export schematics to 'sch.pdf'...
  => 'sch.pdf'
SUCCESS
""")
    assert code == 0
    assert os.path.exists(path)


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
    ],
)
def test_exporting_pdf_with_absolute_path(cli, project):
    """
    Note: Test with passing the argument as "--arg <value>".
    """
    cli.suppress_deprecation_warnings = True
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    path = cli.abspath("schematic with spaces.pdf")
    assert not os.path.exists(path)
    code, stdout, stderr = cli.run(
        "open-project", "--export-schematics", path, project.path
    )
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export schematics to '{path}'...
  => '{path}'
SUCCESS
""")
    assert code == 0
    assert os.path.exists(path)


@pytest.mark.parametrize(
    "file_extension",
    [
        "svg",
        "png",
    ],
)
def test_exporting_images(cli, file_extension):
    cli.suppress_deprecation_warnings = True
    project = params.PROJECT_WITH_TWO_BOARDS_LPPZ
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    path = cli.abspath("schematic with spaces." + file_extension)
    assert not os.path.exists(path)
    code, stdout, stderr = cli.run(
        "open-project", "--export-schematics={}".format(path), project.path
    )
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export schematics to '{path}'...
  => '{path}'
SUCCESS
""")
    assert code == 0
    assert os.path.exists(path)


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
    ],
)
def test_if_output_directories_are_created(cli, project):
    cli.suppress_deprecation_warnings = True
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath("nonexistent directory/nested")
    path = os.path.join(dir, "schematic.pdf")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project", "--export-schematics={}".format(path), project.path
    )
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export schematics to '{path}'...
  => '{path}'
SUCCESS
""")
    assert code == 0
    assert os.path.exists(dir)
    assert os.path.exists(path)


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
    ],
)
def test_deprecation_warning(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    path = cli.abspath("sch.pdf")
    assert not os.path.exists(path)
    code, stdout, stderr = cli.run(
        "open-project", "--export-schematics=sch.pdf", project.path
    )
    assert stderr == nofmt("""\
WARNING: The command or option '--export-schematics' is deprecated and will \
be removed in a future release. Please see '--run-jobs' for a possible \
replacement. For now, the command will be executed, but the CLI will return \
with a nonzero exit code. As a temporary workaround, this warning and the \
nonzero exit code can be suppressed with the environment variable \
'LIBREPCB_SUPPRESS_DEPRECATION_WARNINGS=1'.
""")
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export schematics to 'sch.pdf'...
  => 'sch.pdf'
Finished with warnings!
""")
    assert code == 2
    assert os.path.exists(path)
