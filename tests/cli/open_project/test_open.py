#!/usr/bin/env python
# -*- coding: utf-8 -*-

import params
import pytest
from helpers import nofmt

"""
Test command "open-project"
"""


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_open_project_absolute_path(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project', cli.abspath(project.path))
    assert stderr == ''
    assert stdout == nofmt(f"""\
Open project '{cli.abspath(project.path)}'...
SUCCESS
""")
    assert code == 0


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_open_project_relative_path(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project', project.path)
    assert stderr == ''
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
SUCCESS
""")
    assert code == 0


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_open_project_verbose(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project', '--verbose', project.path)
    assert len(stderr) > 100  # logging messages are on stderr
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
SUCCESS
""")
    assert code == 0
