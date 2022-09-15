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
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export PCB fabrication data...\n" \
        "SUCCESS\n".format(project=project)
    assert code == 0
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
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export PCB fabrication data...\n" \
        "  Board 'default':\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0
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
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export PCB fabrication data...\n" \
        "  Board 'default':\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 9


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_if_exporting_invalid_board_fails(cli, project):
    """
    Note: Test with passing the argument as "--arg <value>".
    """
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + '/gerber')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--board', 'foo',
                                   project.path)
    assert stderr == "ERROR: No board with the name 'foo' found.\n"
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export PCB fabrication data...\n" \
        "Finished with errors!\n".format(project=project)
    assert code == 1
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
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export PCB fabrication data...\n" \
        "  Board 'default':\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr'\n" \
        "  Board 'copy':\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_DRILLS-NPTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_DRILLS-PTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_OUTLINES.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_COPPER-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_COPPER-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_SOLDERMASK-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_SOLDERMASK-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_SILKSCREEN-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_SILKSCREEN-BOTTOM.gbr'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0
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
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export PCB fabrication data...\n" \
        "  Board 'copy':\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_DRILLS-NPTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_DRILLS-PTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_OUTLINES.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_COPPER-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_COPPER-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_SOLDERMASK-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_SOLDERMASK-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_SILKSCREEN-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_SILKSCREEN-BOTTOM.gbr'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0
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
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export PCB fabrication data...\n" \
        "  Board 'copy':\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_DRILLS-NPTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_DRILLS-PTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_OUTLINES.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_COPPER-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_COPPER-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_SOLDERMASK-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_SOLDERMASK-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_SILKSCREEN-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_copy_SILKSCREEN-BOTTOM.gbr'\n" \
        "  Board 'default':\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0
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
    assert stderr == \
        "ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr' was written multiple times!\n" \
        "ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr' was written multiple times!\n" \
        "ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl' was written multiple times!\n" \
        "ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl' was written multiple times!\n" \
        "ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr' was written multiple times!\n" \
        "ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr' was written multiple times!\n" \
        "ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr' was written multiple times!\n" \
        "ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr' was written multiple times!\n" \
        "ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr' was written multiple times!\n" \
        "NOTE: To avoid writing files multiple times, make sure to pass " \
        "unique filepaths to all export functions. For board output files, " \
        "you could either add the placeholder '{{{{BOARD}}}}' to the path or " \
        "specify the boards to export with the '--board' argument.\n"\
        .format(project=project).replace('//', os.sep)
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export PCB fabrication data...\n" \
        "  Board 'default':\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr'\n" \
        "  Board 'copy':\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr'\n" \
        "Finished with errors!\n".format(project=project).replace('//', os.sep)
    assert code == 1


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
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export PCB fabrication data...\n" \
        "  Board 'copy':\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr'\n" \
        "    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 9


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.EMPTY_PROJECT_LPPZ_PARAM,
])
def test_export_with_custom_settings(cli, project):
    """
    Notes:
      - Test with passing the argument as "--arg <value>".
      - Test attributes '{{BOARD_INDEX}}' and '{{BOARD_DIRNAME}}' in path.
    """
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    settings = """
      (fabrication_output_settings
        (base_path "./out/{{BOARD_INDEX}}_{{BOARD_DIRNAME}}/")
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
    dir = os.path.dirname(cli.abspath(project.path)) + '/out/0_default'
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pcb-fabrication-data',
                                   '--pcb-fabrication-settings', 'settings.lp',
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export PCB fabrication data...\n" \
        "  Board 'default':\n" \
        "    => '{output_prefix}out//0_default//DRILLS.drl'\n" \
        "    => '{output_prefix}out//0_default//OUTLINES.gbr'\n" \
        "    => '{output_prefix}out//0_default//COPPER-TOP.gbr'\n" \
        "    => '{output_prefix}out//0_default//COPPER-BOTTOM.gbr'\n" \
        "    => '{output_prefix}out//0_default//SOLDERMASK-TOP.gbr'\n" \
        "    => '{output_prefix}out//0_default//SOLDERMASK-BOTTOM.gbr'\n" \
        "    => '{output_prefix}out//0_default//SILKSCREEN-TOP.gbr'\n" \
        "    => '{output_prefix}out//0_default//SILKSCREEN-BOTTOM.gbr'\n" \
        "    => '{output_prefix}out//0_default//SOLDERPASTE-TOP.gbr'\n" \
        "    => '{output_prefix}out//0_default//SOLDERPASTE-BOTTOM.gbr'\n" \
        "SUCCESS\n".format(
            project=project,
            output_prefix=('' if project.is_lppz else (project.dir + os.sep)),
        ).replace('//', os.sep)
    assert code == 0
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
    assert stderr == \
        "ERROR: Failed to load custom settings: " \
        "The file \"{file}\" does not exist.\n" \
        .format(file=cli.abspath('nonexistent.lp'))
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export PCB fabrication data...\n" \
        "Finished with errors!\n".format(project=project)
    assert code == 1
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
    assert stderr == \
        "ERROR: Failed to load custom settings: File parse error: " \
        "Child not found: base_path/@0\n\n" \
        "File: \n" \
        "Line,Column: -1,-1\n" \
        "Invalid Content: \"\"\n"
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export PCB fabrication data...\n" \
        "Finished with errors!\n".format(project=project)
    assert code == 1
    assert not os.path.exists(dir)
