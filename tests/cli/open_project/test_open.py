#!/usr/bin/env python
# -*- coding: utf-8 -*-

import params
import pytest

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
    assert stdout == \
        "Open project '{path}'...\n" \
        "SUCCESS\n".format(path=cli.abspath(project.path))
    assert code == 0


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_open_project_relative_path(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project', project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "SUCCESS\n".format(project=project)
    assert code == 0


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_open_project_verbose(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project', '--verbose', project.path)
    assert len(stderr) > 100  # logging messages are on stderr
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "SUCCESS\n".format(project=project)
    assert code == 0
