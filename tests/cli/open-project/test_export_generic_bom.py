#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import pytest

"""
Test command "open-project --export-bom"
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
                                   '--export-bom=' + relpath,
                                   PROJECT_PATH_1_LPP)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(abspath)


@pytest.mark.parametrize("project,output_dir", [
    (PROJECT_PATH_2_LPP, OUTPUT_DIR_2_LPP),
    (PROJECT_PATH_2_LPPZ, OUTPUT_DIR_2_LPPZ),
], ids=[
    'ProjectWithTwoBoards.lpp',
    'ProjectWithTwoBoards.lppz',
])
def test_export_multiple_files(cli, project, output_dir):
    relpath1 = output_dir + 'bom1.csv'
    abspath1 = cli.abspath(relpath1)
    assert not os.path.exists(abspath1)
    relpath2 = output_dir + 'bom2.csv'
    abspath2 = cli.abspath(relpath2)
    assert not os.path.exists(abspath2)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-bom=' + relpath1,
                                   '--export-bom=' + relpath2,
                                   project)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(abspath1)
    assert os.path.exists(abspath2)
