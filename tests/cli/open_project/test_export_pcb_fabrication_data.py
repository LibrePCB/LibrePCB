#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import fileinput
import params
import pytest
from helpers import nofmt

"""
Test command "open-project --export-pcb-fabrication-data"
"""


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    ],
)
def test_if_project_without_boards_succeeds(cli, project):
    cli.suppress_deprecation_warnings = True
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    # remove all boards first
    with open(cli.abspath(project.dir + "/boards/boards.lp"), "w") as f:
        f.write("(librepcb_boards)")

    dir = cli.abspath(project.output_dir + "/gerber")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project", "--export-pcb-fabrication-data", project.path
    )
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export PCB fabrication data...
SUCCESS
""")
    assert code == 0
    assert not os.path.exists(dir)  # nothing was exported ;)


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
        params.EMPTY_PROJECT_LPPZ_PARAM,
    ],
)
def test_export_project_with_one_board_implicit(cli, project):
    cli.suppress_deprecation_warnings = True
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + "/gerber")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project", "--export-pcb-fabrication-data", project.path
    )
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export PCB fabrication data...
  Board 'default':
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr'
SUCCESS
""").replace("//", os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 9


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
        params.EMPTY_PROJECT_LPPZ_PARAM,
    ],
)
def test_export_project_with_one_board_explicit(cli, project):
    cli.suppress_deprecation_warnings = True
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + "/gerber")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project", "--export-pcb-fabrication-data", "--board=default", project.path
    )
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export PCB fabrication data...
  Board 'default':
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr'
SUCCESS
""").replace("//", os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 9


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
    ],
)
def test_if_exporting_invalid_board_fails(cli, project):
    """
    Note: Test with passing the argument as "--arg <value>".
    """
    cli.suppress_deprecation_warnings = True
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + "/gerber")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project", "--export-pcb-fabrication-data", "--board", "foo", project.path
    )
    assert stderr == "ERROR: No board with the name 'foo' found.\n"
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export PCB fabrication data...
Finished with errors!
""")
    assert code == 1
    assert not os.path.exists(dir)


@pytest.mark.parametrize(
    "project",
    [
        params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
    ],
)
def test_export_project_with_two_boards_implicit(cli, project):
    cli.suppress_deprecation_warnings = True
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + "/gerber")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project", "--export-pcb-fabrication-data", project.path
    )
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export PCB fabrication data...
  Board 'default':
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr'
  Board 'copy':
    => '{project.output_dir_native}//gerber//Empty_Project_copy_DRILLS-NPTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_DRILLS-PTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_OUTLINES.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_COPPER-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_COPPER-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_SOLDERMASK-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_SOLDERMASK-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_SILKSCREEN-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_SILKSCREEN-BOTTOM.gbr'
SUCCESS
""").replace("//", os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 18


@pytest.mark.parametrize(
    "project",
    [
        params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
    ],
)
def test_export_project_with_two_boards_explicit_one(cli, project):
    cli.suppress_deprecation_warnings = True
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + "/gerber")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project", "--export-pcb-fabrication-data", "--board=copy", project.path
    )
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export PCB fabrication data...
  Board 'copy':
    => '{project.output_dir_native}//gerber//Empty_Project_copy_DRILLS-NPTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_DRILLS-PTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_OUTLINES.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_COPPER-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_COPPER-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_SOLDERMASK-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_SOLDERMASK-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_SILKSCREEN-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_SILKSCREEN-BOTTOM.gbr'
SUCCESS
""").replace("//", os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 9


@pytest.mark.parametrize(
    "project",
    [
        params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
    ],
)
def test_export_project_with_two_boards_explicit_two(cli, project):
    cli.suppress_deprecation_warnings = True
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + "/gerber")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project",
        "--export-pcb-fabrication-data",
        "--board=copy",
        "--board=default",
        project.path,
    )
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export PCB fabrication data...
  Board 'copy':
    => '{project.output_dir_native}//gerber//Empty_Project_copy_DRILLS-NPTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_DRILLS-PTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_OUTLINES.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_COPPER-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_COPPER-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_SOLDERMASK-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_SOLDERMASK-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_SILKSCREEN-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_copy_SILKSCREEN-BOTTOM.gbr'
  Board 'default':
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr'
SUCCESS
""").replace("//", os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 18


@pytest.mark.parametrize(
    "project",
    [
        params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    ],
)
def test_export_project_with_two_conflicting_boards_fails(cli, project):
    cli.suppress_deprecation_warnings = True
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    # change gerber output path to the same for both boards
    boardfile = cli.abspath(project.dir + "/boards/copy/board.lp")
    for line in fileinput.input(boardfile, inplace=1):
        print(line.replace("_copy", ""))

    dir = cli.abspath(project.output_dir + "/gerber")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project", "--export-pcb-fabrication-data", project.path
    )
    assert stderr == nofmt(f"""\
ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr' was written multiple times!
ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr' was written multiple times!
ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl' was written multiple times!
ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl' was written multiple times!
ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr' was written multiple times!
ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr' was written multiple times!
ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr' was written multiple times!
ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr' was written multiple times!
ERROR: The file '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr' was written multiple times!
NOTE: To avoid writing files multiple times, make sure to pass \
unique filepaths to all export functions. For board output files, \
you could either add the placeholder '{{{{BOARD}}}}' to the path or \
specify the boards to export with the '--board' argument.
""").replace("//", os.sep)
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export PCB fabrication data...
  Board 'default':
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr'
  Board 'copy':
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr'
Finished with errors!
""").replace("//", os.sep)
    assert code == 1


@pytest.mark.parametrize(
    "project",
    [
        params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    ],
)
def test_export_project_with_two_conflicting_boards_succeeds_explicit(cli, project):
    cli.suppress_deprecation_warnings = True
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    # change gerber output path to the same for both boards
    boardfile = cli.abspath(project.dir + "/boards/copy/board.lp")
    for line in fileinput.input(boardfile, inplace=1):
        print(line.replace("_copy", ""))

    dir = cli.abspath(project.output_dir + "/gerber")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project", "--export-pcb-fabrication-data", "--board=copy", project.path
    )
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export PCB fabrication data...
  Board 'copy':
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr'
SUCCESS
""").replace("//", os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 9


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
        params.EMPTY_PROJECT_LPPZ_PARAM,
    ],
)
def test_export_with_custom_settings(cli, project):
    """
    Notes:
      - Test with passing the argument as "--arg <value>".
      - Test attributes '{{BOARD_INDEX}}' and '{{BOARD_DIRNAME}}' in path.
    """
    cli.suppress_deprecation_warnings = True
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
          (suffix_buried "_DRILLS-{{START_LAYER}}-{{END_LAYER}}.drl")
          (g85_slots false)
        )
        (solderpaste_top (create true) (suffix "SOLDERPASTE-TOP.gbr"))
        (solderpaste_bot (create true) (suffix "SOLDERPASTE-BOTTOM.gbr"))
      )
    """
    with open(cli.abspath("settings.lp"), mode="w") as f:
        f.write(settings)
    dir = os.path.dirname(cli.abspath(project.path)) + "/out/0_default"
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project",
        "--export-pcb-fabrication-data",
        "--pcb-fabrication-settings",
        "settings.lp",
        project.path,
    )
    output_prefix = "" if project.is_lppz else (project.dir + os.sep)
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export PCB fabrication data...
  Board 'default':
    => '{output_prefix}out//0_default//DRILLS.drl'
    => '{output_prefix}out//0_default//OUTLINES.gbr'
    => '{output_prefix}out//0_default//COPPER-TOP.gbr'
    => '{output_prefix}out//0_default//COPPER-BOTTOM.gbr'
    => '{output_prefix}out//0_default//SOLDERMASK-TOP.gbr'
    => '{output_prefix}out//0_default//SOLDERMASK-BOTTOM.gbr'
    => '{output_prefix}out//0_default//SILKSCREEN-TOP.gbr'
    => '{output_prefix}out//0_default//SILKSCREEN-BOTTOM.gbr'
    => '{output_prefix}out//0_default//SOLDERPASTE-TOP.gbr'
    => '{output_prefix}out//0_default//SOLDERPASTE-BOTTOM.gbr'
SUCCESS
""").replace("//", os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 10


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
    ],
)
def test_if_export_with_nonexistent_settings_fails(cli, project):
    cli.suppress_deprecation_warnings = True
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + "/gerber")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project",
        "--export-pcb-fabrication-data",
        "--pcb-fabrication-settings=nonexistent.lp",
        project.path,
    )
    assert stderr == nofmt(f"""\
ERROR: Failed to load custom settings: \
The file "{cli.abspath("nonexistent.lp")}" does not exist.
""")
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export PCB fabrication data...
Finished with errors!
""")
    assert code == 1
    assert not os.path.exists(dir)


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
    ],
)
def test_if_export_with_invalid_settings_fails(cli, project):
    cli.suppress_deprecation_warnings = True
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    with open(cli.abspath("settings.lp"), mode="w") as f:
        f.write("foobar")
    dir = cli.abspath(project.output_dir + "/gerber")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project",
        "--export-pcb-fabrication-data",
        "--pcb-fabrication-settings=settings.lp",
        project.path,
    )
    assert stderr == nofmt("""\
ERROR: Failed to load custom settings: File parse error: Child not found: base_path/@0
File: \n\
Invalid Content: ''
""")
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export PCB fabrication data...
Finished with errors!
""")
    assert code == 1
    assert not os.path.exists(dir)


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
    ],
)
def test_deprecation_warning(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir + "/gerber")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project", "--export-pcb-fabrication-data", project.path
    )
    assert stderr == nofmt("""\
WARNING: The command or option '--export-pcb-fabrication-data' is deprecated \
and will be removed in a future release. Please see '--run-jobs' for a \
possible replacement. For now, the command will be executed, but the CLI will \
return with a nonzero exit code. As a temporary workaround, this warning and \
the nonzero exit code can be suppressed with the environment variable \
'LIBREPCB_SUPPRESS_DEPRECATION_WARNINGS=1'.
""")
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Export PCB fabrication data...
  Board 'default':
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-NPTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_DRILLS-PTH.drl'
    => '{project.output_dir_native}//gerber//Empty_Project_OUTLINES.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_COPPER-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SOLDERMASK-BOTTOM.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-TOP.gbr'
    => '{project.output_dir_native}//gerber//Empty_Project_SILKSCREEN-BOTTOM.gbr'
Finished with warnings!
""").replace("//", os.sep)
    assert code == 2
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 9
