#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import fileinput

"""
Test command "open-project --export-pcb-fabrication-data"
"""

PROJECT_DIR = 'data/Empty Project/'
PROJECT_PATH = PROJECT_DIR + 'Empty Project.lpp'
OUTPUT_DIR = PROJECT_DIR + 'output/v1/gerber'

PROJECT_2_DIR = 'data/Project With Two Boards/'
PROJECT_2_PATH = PROJECT_2_DIR + 'Project With Two Boards.lpp'
OUTPUT_2_DIR = PROJECT_2_DIR + 'output/v1/gerber'


def test_if_project_without_boards_succeeds(cli):
    # remove all boards first
    with open(cli.abspath(PROJECT_DIR + 'boards/boards.lp'), 'w') as f:
        f.write('(librepcb_boards)')
    dir = cli.abspath(OUTPUT_DIR)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   PROJECT_PATH)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert not os.path.exists(dir)  # nothing was exported ;)


def test_export_project_with_one_board_implicit(cli):
    dir = cli.abspath(OUTPUT_DIR)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   PROJECT_PATH)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 8


def test_export_project_with_one_board_explicit(cli):
    dir = cli.abspath(OUTPUT_DIR)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board=default',
                                   PROJECT_PATH)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 8


def test_if_exporting_invalid_board_fails(cli):
    dir = cli.abspath(OUTPUT_DIR)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board=foo',
                                   PROJECT_PATH)
    assert code == 1
    assert len(stderr) == 1
    assert "No board with the name 'foo' found." in stderr[0]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'
    assert not os.path.exists(dir)


def test_export_project_with_two_boards_implicit(cli):
    dir = cli.abspath(OUTPUT_2_DIR)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   PROJECT_2_PATH)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 16


def test_export_project_with_two_boards_explicit_one(cli):
    dir = cli.abspath(OUTPUT_2_DIR)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board=copy',
                                   PROJECT_2_PATH)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 8


def test_export_project_with_two_boards_explicit_two(cli):
    dir = cli.abspath(OUTPUT_2_DIR)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board=copy',
                                   '--board=default',
                                   PROJECT_2_PATH)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 16


def test_export_project_with_two_conflicting_boards_fails(cli):
    # change gerber output path to the same for both boards
    boardfile = cli.abspath(PROJECT_2_DIR + 'boards/copy/board.lp')
    for line in fileinput.input(boardfile, inplace=1):
        print(line.replace("_copy", ""))
    dir = cli.abspath(OUTPUT_2_DIR)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   PROJECT_2_PATH)
    assert code == 1
    assert len(stderr) > 0
    assert 'Some files were written multiple times' in stderr[0]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'


def test_export_project_with_two_conflicting_boards_succeeds_explicit(cli):
    # change gerber output path to the same for both boards
    boardfile = cli.abspath(PROJECT_2_DIR + 'boards/copy/board.lp')
    for line in fileinput.input(boardfile, inplace=1):
        print(line.replace("_copy", ""))
    dir = cli.abspath(OUTPUT_2_DIR)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board=copy',
                                   PROJECT_2_PATH)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 8
