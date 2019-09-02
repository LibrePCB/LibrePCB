#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
import pytest

"""
Test command "open-project --export-board-bom"
"""


@pytest.mark.parametrize("project", [params.EMPTY_PROJECT_LPP_PARAM])
def test_if_project_without_boards_succeeds(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    # remove all boards first
    with open(cli.abspath(project.dir + '/boards/boards.lp'), 'w') as f:
        f.write('(librepcb_boards)')

    relpath = project.output_dir + 'bom/bom.csv'
    abspath = cli.abspath(relpath)
    assert not os.path.exists(abspath)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom=' + relpath,
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert not os.path.exists(abspath)  # nothing exported


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_export_project_with_two_boards_implicit(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    fp = project.output_dir + '/bom/{{BOARD}}.csv'
    dir = cli.abspath(project.output_dir + '/bom')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom=' + fp,
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 2


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_export_project_with_two_boards_explicit_one(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    fp = project.output_dir + '/bom/{{BOARD}}.csv'
    dir = cli.abspath(project.output_dir + '/bom')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom=' + fp,
                                   '--board=copy',
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 1


@pytest.mark.parametrize("project", [params.PROJECT_WITH_TWO_BOARDS_LPP])
def test_export_project_with_two_conflicting_boards_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    fp = project.output_dir + '/bom.csv'
    dir = cli.abspath(project.output_dir + '/bom')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom=' + fp,
                                   project.path)
    assert code == 1
    assert len(stderr) > 0
    assert 'was written multiple times' in stderr[0]
    assert 'NOTE: To avoid writing files multiple times,' in stderr[-1]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'
