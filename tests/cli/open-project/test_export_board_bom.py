#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import pytest

"""
Test command "open-project --export-board-bom"
"""

PROJECT_DIR_1 = 'data/Empty Project'
PROJECT_PATH_1_LPP = PROJECT_DIR_1 + '/Empty Project.lpp'
PROJECT_PATH_1_LPPZ = PROJECT_DIR_1 + '.lppz'
OUTPUT_DIR_1_LPP = PROJECT_DIR_1 + '/output/v1/bom'
OUTPUT_DIR_1_LPPZ = 'data/output/v1/bom'

PROJECT_DIR_2 = 'data/Project With Two Boards'
PROJECT_PATH_2_LPP = PROJECT_DIR_2 + '/Project With Two Boards.lpp'
PROJECT_PATH_2_LPPZ = PROJECT_DIR_2 + '.lppz'
OUTPUT_DIR_2_LPP = PROJECT_DIR_2 + '/output/v1/bom'
OUTPUT_DIR_2_LPPZ = 'data/output/v1/bom'


def test_if_project_without_boards_succeeds(cli):
    # remove all boards first
    with open(cli.abspath(PROJECT_DIR_1 + '/boards/boards.lp'), 'w') as f:
        f.write('(librepcb_boards)')
    relpath = OUTPUT_DIR_1_LPP + 'bom.csv'
    abspath = cli.abspath(relpath)
    assert not os.path.exists(abspath)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom=' + relpath,
                                   PROJECT_PATH_1_LPP)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert not os.path.exists(abspath)  # nothing exported


@pytest.mark.parametrize("project,output_dir", [
    (PROJECT_PATH_2_LPP, OUTPUT_DIR_2_LPP),
    (PROJECT_PATH_2_LPPZ, OUTPUT_DIR_2_LPPZ),
], ids=[
    'ProjectWithTwoBoards.lpp',
    'ProjectWithTwoBoards.lppz',
])
def test_export_project_with_two_boards_implicit(cli, project, output_dir):
    fp = output_dir + r'/{{BOARD}}.csv'
    dir = cli.abspath(output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom=' + fp,
                                   project)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 2


@pytest.mark.parametrize("project,output_dir", [
    (PROJECT_PATH_2_LPP, OUTPUT_DIR_2_LPP),
    (PROJECT_PATH_2_LPPZ, OUTPUT_DIR_2_LPPZ),
], ids=[
    'ProjectWithTwoBoards.lpp',
    'ProjectWithTwoBoards.lppz',
])
def test_export_project_with_two_boards_explicit_one(cli, project, output_dir):
    fp = output_dir + r'/{{BOARD}}.csv'
    dir = cli.abspath(output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom=' + fp,
                                   '--board=copy',
                                   project)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 1


def test_export_project_with_two_conflicting_boards_fails(cli):
    fp = OUTPUT_DIR_2_LPP + '/bom.csv'
    dir = cli.abspath(OUTPUT_DIR_2_LPP)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom=' + fp,
                                   PROJECT_PATH_2_LPP)
    assert code == 1
    assert len(stderr) > 0
    assert 'was written multiple times' in stderr[0]
    assert 'NOTE: To avoid writing files multiple times,' in stderr[-1]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'
