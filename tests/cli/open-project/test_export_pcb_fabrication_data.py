#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import fileinput
import params
import pytest

"""
Test command "open-project --export-pcb-fabrication-data"
"""


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
])
def test_if_project_without_boards_succeeds(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    # remove all boards first
    with open(cli.abspath(project.dir + '/boards/boards.lp'), 'w') as f:
        f.write('(librepcb_boards)')

    dir = cli.abspath(project.output_dir + '/gerber')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert not os.path.exists(dir)  # nothing was exported ;)


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.EMPTY_PROJECT_LPPZ_PARAM,
])
def test_export_project_with_one_board_implicit(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + '/gerber')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 9


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.EMPTY_PROJECT_LPPZ_PARAM,
])
def test_export_project_with_one_board_explicit(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + '/gerber')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board=default',
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 9


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_if_exporting_invalid_board_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + '/gerber')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board=foo',
                                   project.path)
    assert code == 1
    assert len(stderr) == 1
    assert "No board with the name 'foo' found." in stderr[0]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'
    assert not os.path.exists(dir)


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_export_project_with_two_boards_implicit(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + '/gerber')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 18


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_export_project_with_two_boards_explicit_one(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + '/gerber')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board=copy',
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 9


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_export_project_with_two_boards_explicit_two(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + '/gerber')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board=copy',
                                   '--board=default',
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 18


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
])
def test_export_project_with_two_conflicting_boards_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    # change gerber output path to the same for both boards
    boardfile = cli.abspath(project.dir + '/boards/copy/board.lp')
    for line in fileinput.input(boardfile, inplace=1):
        print(line.replace("_copy", ""))

    dir = cli.abspath(project.output_dir + '/gerber')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   project.path)
    assert code == 1
    assert len(stderr) > 0
    assert 'was written multiple times' in stderr[0]
    assert 'NOTE: To avoid writing files multiple times,' in stderr[-1]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
])
def test_export_project_with_two_conflicting_boards_succeeds_explicit(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    # change gerber output path to the same for both boards
    boardfile = cli.abspath(project.dir + '/boards/copy/board.lp')
    for line in fileinput.input(boardfile, inplace=1):
        print(line.replace("_copy", ""))

    dir = cli.abspath(project.output_dir + '/gerber')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board=copy',
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 9


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.EMPTY_PROJECT_LPPZ_PARAM,
])
def test_export_with_custom_settings(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
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
    dir = os.path.dirname(cli.abspath(project.path)) + '/custom_output'
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--pcb-fabrication-settings=settings.lp',
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 10


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_if_export_with_nonexistent_settings_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + '/gerber')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--pcb-fabrication-settings=nonexistent.lp',
                                   project.path)
    assert code == 1
    assert len(stderr) == 1
    assert "Failed to load custom settings:" in stderr[0]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'
    assert not os.path.exists(dir)


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_if_export_with_invalid_settings_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    with open(cli.abspath('settings.lp'), mode='w') as f:
        f.write('foobar')
    dir = cli.abspath(project.output_dir + '/gerber')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--pcb-fabrication-settings=settings.lp',
                                   project.path)
    assert code == 1
    assert len(stderr) > 0
    assert "Failed to load custom settings: File parse error:" in stderr[0]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'
    assert not os.path.exists(dir)
