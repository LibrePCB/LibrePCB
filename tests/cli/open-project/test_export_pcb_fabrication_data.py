#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import fileinput
import pytest

"""
Test command "open-project --export-pcb-fabrication-data"
"""

PROJECT_DIR_1 = 'data/Empty Project'
PROJECT_PATH_1_LPP = PROJECT_DIR_1 + '/Empty Project.lpp'
PROJECT_PATH_1_LPPZ = PROJECT_DIR_1 + '.lppz'
OUTPUT_DIR_1_LPP = PROJECT_DIR_1 + '/output/v1/gerber'
OUTPUT_DIR_1_LPPZ = 'data/output/v1/gerber'

PROJECT_DIR_2 = 'data/Project With Two Boards'
PROJECT_PATH_2_LPP = PROJECT_DIR_2 + '/Project With Two Boards.lpp'
PROJECT_PATH_2_LPPZ = PROJECT_DIR_2 + '.lppz'
OUTPUT_DIR_2_LPP = PROJECT_DIR_2 + '/output/v1/gerber'
OUTPUT_DIR_2_LPPZ = 'data/output/v1/gerber'


@pytest.mark.parametrize("project", [
    'Empty Project',
    'Project With Two Boards',
], ids=[
    'EmptyProject.lpp',
    'ProjectWithTwoBoards.lpp',
])
def test_if_project_without_boards_succeeds(cli, project):
    # remove all boards first
    with open(cli.abspath('data/' + project + '/boards/boards.lp'), 'w') as f:
        f.write('(librepcb_boards)')
    dir = cli.abspath('data/' + project + '/output/v1/gerber')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   'data/' + project + '/' + project + '.lpp')
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert not os.path.exists(dir)  # nothing was exported ;)


@pytest.mark.parametrize("project,output_dir", [
    (PROJECT_PATH_1_LPP, OUTPUT_DIR_1_LPP),
    (PROJECT_PATH_1_LPPZ, OUTPUT_DIR_1_LPPZ),
], ids=[
    'EmptyProject.lpp',
    'EmptyProject.lppz',
])
def test_export_project_with_one_board_implicit(cli, project, output_dir):
    dir = cli.abspath(output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   project)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 8


@pytest.mark.parametrize("project,output_dir", [
    (PROJECT_PATH_1_LPP, OUTPUT_DIR_1_LPP),
    (PROJECT_PATH_1_LPPZ, OUTPUT_DIR_1_LPPZ),
], ids=[
    'EmptyProject.lpp',
    'EmptyProject.lppz',
])
def test_export_project_with_one_board_explicit(cli, project, output_dir):
    dir = cli.abspath(output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board=default',
                                   project)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 8


@pytest.mark.parametrize("project,output_dir", [
    (PROJECT_PATH_1_LPP, OUTPUT_DIR_1_LPP),
    (PROJECT_PATH_2_LPPZ, OUTPUT_DIR_2_LPPZ),
], ids=[
    'EmptyProject.lpp',
    'ProjectWithTwoBoards.lppz',
])
def test_if_exporting_invalid_board_fails(cli, project, output_dir):
    dir = cli.abspath(output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board=foo',
                                   project)
    assert code == 1
    assert len(stderr) == 1
    assert "No board with the name 'foo' found." in stderr[0]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'
    assert not os.path.exists(dir)


@pytest.mark.parametrize("project,output_dir", [
    (PROJECT_PATH_2_LPP, OUTPUT_DIR_2_LPP),
    (PROJECT_PATH_2_LPPZ, OUTPUT_DIR_2_LPPZ),
], ids=[
    'ProjectWithTwoBoards.lpp',
    'ProjectWithTwoBoards.lppz',
])
def test_export_project_with_two_boards_implicit(cli, project, output_dir):
    dir = cli.abspath(output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   project)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 16


@pytest.mark.parametrize("project,output_dir", [
    (PROJECT_PATH_2_LPP, OUTPUT_DIR_2_LPP),
    (PROJECT_PATH_2_LPPZ, OUTPUT_DIR_2_LPPZ),
], ids=[
    'ProjectWithTwoBoards.lpp',
    'ProjectWithTwoBoards.lppz',
])
def test_export_project_with_two_boards_explicit_one(cli, project, output_dir):
    dir = cli.abspath(output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board=copy',
                                   project)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 8


@pytest.mark.parametrize("project,output_dir", [
    (PROJECT_PATH_2_LPP, OUTPUT_DIR_2_LPP),
    (PROJECT_PATH_2_LPPZ, OUTPUT_DIR_2_LPPZ),
], ids=[
    'ProjectWithTwoBoards.lpp',
    'ProjectWithTwoBoards.lppz',
])
def test_export_project_with_two_boards_explicit_two(cli, project, output_dir):
    dir = cli.abspath(output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board=copy',
                                   '--board=default',
                                   project)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 16


def test_export_project_with_two_conflicting_boards_fails(cli):
    # change gerber output path to the same for both boards
    boardfile = cli.abspath(PROJECT_DIR_2 + '/boards/copy/board.lp')
    for line in fileinput.input(boardfile, inplace=1):
        print(line.replace("_copy", ""))
    dir = cli.abspath(OUTPUT_DIR_2_LPP)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   PROJECT_PATH_2_LPP)
    assert code == 1
    assert len(stderr) > 0
    assert 'Some files were written multiple times' in stderr[0]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'


def test_export_project_with_two_conflicting_boards_succeeds_explicit(cli):
    # change gerber output path to the same for both boards
    boardfile = cli.abspath(PROJECT_DIR_2 + '/boards/copy/board.lp')
    for line in fileinput.input(boardfile, inplace=1):
        print(line.replace("_copy", ""))
    dir = cli.abspath(OUTPUT_DIR_2_LPP)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board=copy',
                                   PROJECT_PATH_2_LPP)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 8


@pytest.mark.parametrize("project,output_dir", [
    (PROJECT_PATH_1_LPP, 'data/Empty Project/custom_output'),
    (PROJECT_PATH_1_LPPZ, 'data/custom_output'),
], ids=[
    'EmptyProject.lpp',
    'EmptyProject.lppz',
])
def test_export_with_custom_settings(cli, project, output_dir):
    settings = """
      (fabrication_output_settings
        (base_path "./custom_output/")
        (outlines (suffix "OUTLINES.gbr"))
        (copper_top (suffix "COPPER-TOP.gbr"))
        (copper_inner (suffix "COPPER-IN{{CU_LAYER}}.gbr"))
        (copper_bot (suffix "COPPER-BOTTOM.gbr"))
        (soldermask_top (suffix "SOLDERMASK-TOP.gbr"))
        (soldermask_bot (suffix "SOLDERMASK-BOTTOM.gbr"))
        (silkscreen_top (suffix "SILKSCREEN-TOP.gbr")
          (layers top_placement top_names)
        )
        (silkscreen_bot (suffix "SILKSCREEN-BOTTOM.gbr")
          (layers bot_placement bot_names)
        )
        (drills (merge true)
          (suffix_pth "DRILLS-PTH.drl")
          (suffix_npth "DRILLS-NPTH.drl")
          (suffix_merged "DRILLS.drl")
        )
        (solderpaste_top (create true) (suffix "SOLDERPASTE-TOP.gbr"))
        (solderpaste_bot (create true) (suffix "SOLDERPASTE-BOTTOM.gbr"))
      )
    """
    with open(cli.abspath('settings.lp'), mode='w') as f:
        f.write(settings)
    dir = cli.abspath(output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--pcb-fabrication-settings=settings.lp',
                                   project)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 10


@pytest.mark.parametrize("project,output_dir", [
    (PROJECT_PATH_1_LPP, OUTPUT_DIR_1_LPP),
    (PROJECT_PATH_2_LPPZ, OUTPUT_DIR_2_LPPZ),
], ids=[
    'EmptyProject.lpp',
    'ProjectWithTwoBoards.lppz',
])
def test_if_export_with_nonexistent_settings_fails(cli, project, output_dir):
    dir = cli.abspath(output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--pcb-fabrication-settings=nonexistent.lp',
                                   project)
    assert code == 1
    assert len(stderr) == 1
    assert "Failed to load custom settings:" in stderr[0]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'
    assert not os.path.exists(dir)


@pytest.mark.parametrize("project,output_dir", [
    (PROJECT_PATH_1_LPP, OUTPUT_DIR_1_LPP),
    (PROJECT_PATH_2_LPPZ, OUTPUT_DIR_2_LPPZ),
], ids=[
    'EmptyProject.lpp',
    'ProjectWithTwoBoards.lppz',
])
def test_if_export_with_invalid_settings_fails(cli, project, output_dir):
    with open(cli.abspath('settings.lp'), mode='w') as f:
        f.write('foobar')
    dir = cli.abspath(output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--pcb-fabrication-settings=settings.lp',
                                   project)
    assert code == 1
    assert len(stderr) > 0
    assert "Failed to load custom settings: File parse error:" in stderr[0]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'
    assert not os.path.exists(dir)
