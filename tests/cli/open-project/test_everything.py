#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
import pytest

"""
Test command "open-project" with all available options
"""


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.EMPTY_PROJECT_LPPZ_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_everything(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    # prepare "--export-schematics"
    schematic = cli.abspath('schematic.pdf')
    assert not os.path.exists(schematic)

    # prepare "--export-bom"
    bom = cli.abspath('bom.csv')
    assert not os.path.exists(bom)

    # prepare "--export-board-bom"
    board_bom_dir = cli.abspath(project.output_dir + '/bom')
    assert not os.path.exists(board_bom_dir)
    board_bom = board_bom_dir + r'/{{BOARD}}.csv'

    # prepare "--export-pcb-fabrication-data"
    gerber_dir = cli.abspath(project.output_dir + '/gerber')
    assert not os.path.exists(gerber_dir)

    # prepare "--save"
    path = cli.abspath(project.path)
    with open(path, 'ab') as f:
        f.write(b'\0\0')
    original_filesize = os.path.getsize(path)

    # run the CLI
    code, stdout, stderr = cli.run('open-project',
                                   '--erc',
                                   '--export-schematics={}'.format(schematic),
                                   '--export-bom={}'.format(bom),
                                   '--export-board-bom={}'.format(board_bom),
                                   '--export-pcb-fabrication-data',
                                   '--save',
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'

    # check "--export-schematics"
    assert os.path.exists(schematic)

    # check "--export-bom"
    assert os.path.exists(bom)

    # check "--export-board-bom"
    assert os.path.exists(board_bom_dir)
    assert len(os.listdir(board_bom_dir)) == project.board_count

    # check "--export-pcb-fabrication-data"
    assert os.path.exists(gerber_dir)
    assert len(os.listdir(gerber_dir)) == project.board_count * 8

    # check "--save"
    assert os.path.getsize(path) != original_filesize
