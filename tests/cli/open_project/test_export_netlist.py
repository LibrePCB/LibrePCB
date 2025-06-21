#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
import pytest
from helpers import nofmt

"""
Test command "open-project --export-netlist"
"""


@pytest.mark.parametrize("project", [params.EMPTY_PROJECT_LPP_PARAM])
def test_if_project_without_boards_succeeds(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    # remove all boards first
    with open(cli.abspath(project.dir + "/boards/boards.lp"), "w") as f:
        f.write("(librepcb_boards)")

    relpath = project.output_dir + "/netlist/netlist.d356"
    abspath = cli.abspath(relpath)
    assert not os.path.exists(abspath)
    code, stdout, stderr = cli.run(
        "open-project", "--export-netlist=" + relpath, project.path
    )
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export netlist to 'Empty Project/output/v1/netlist/netlist.d356'...
SUCCESS
""")
    assert code == 0
    assert not os.path.exists(abspath)  # nothing exported


@pytest.mark.parametrize(
    "project",
    [
        params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
    ],
)
def test_export_project_with_two_boards_implicit(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    fp = project.output_dir + "/netlist/{{BOARD}}.d356"
    dir = cli.abspath(project.output_dir + "/netlist")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project", "--export-netlist=" + fp, project.path
    )
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export netlist to '{project.output_dir}/netlist/{{{{BOARD}}}}.d356'...
  - 'default' => '{project.output_dir_native}//netlist//default.d356'
  - 'copy' => '{project.output_dir_native}//netlist//copy.d356'
SUCCESS
""").replace("//", os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 2


@pytest.mark.parametrize(
    "project",
    [
        params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
    ],
)
def test_export_project_with_two_boards_explicit_one(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    fp = project.output_dir + "/netlist/{{BOARD}}.d356"
    dir = cli.abspath(project.output_dir + "/netlist")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project", "--export-netlist", fp, "--board=copy", project.path
    )
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export netlist to '{project.output_dir}/netlist/{{{{BOARD}}}}.d356'...
  - 'copy' => '{project.output_dir_native}//netlist//copy.d356'
SUCCESS
""").replace("//", os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert os.listdir(dir) == ["copy.d356"]


@pytest.mark.parametrize("project", [params.PROJECT_WITH_TWO_BOARDS_LPP])
def test_export_project_with_two_conflicting_boards_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    fp = project.output_dir + "/netlist.d356"
    code, stdout, stderr = cli.run(
        "open-project", "--export-netlist=" + fp, project.path
    )
    assert stderr == nofmt(f"""\
ERROR: The file '{project.output_dir_native}//netlist.d356' was written multiple times!
NOTE: To avoid writing files multiple times, make sure to pass unique filepaths \
to all export functions. For board output files, you could either add the \
placeholder '{{{{BOARD}}}}' to the path or specify the boards to export with \
the '--board' argument.
""").replace("//", os.sep)
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export netlist to '{project.output_dir}/netlist.d356'...
  - 'default' => '{project.output_dir_native}//netlist.d356'
  - 'copy' => '{project.output_dir_native}//netlist.d356'
Finished with errors!
""").replace("//", os.sep)
    assert code == 1
