#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import fileinput
import params
import pytest
from helpers import nofmt

"""
Test command "open-project --run-jobs"
"""


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
    ],
)
def test_if_project_without_jobs_succeeds(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    dir = cli.abspath(project.output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run("open-project", "--run-jobs", project.path)
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
SUCCESS
""")
    assert code == 0
    assert os.path.exists(dir)
    assert os.listdir(dir) == [".librepcb-output"]


@pytest.mark.parametrize(
    "project",
    [
        params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
    ],
)
def test_project_with_jobs(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run("open-project", "--run-jobs", project.path)
    if "LibrePCB was compiled without OpenCascade" in stderr:
        pytest.skip("Feature not available.")
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Run output job 'Schematic PDF'...
  => '{project.output_dir_native}//Empty_Project_v1_Schematic.pdf'
Run output job 'Board Assembly PDF'...
  => '{project.output_dir_native}//Empty_Project_v1_Assembly.pdf'
Run output job 'Board Rendering PDF'...
  => '{project.output_dir_native}//Empty_Project_v1_Rendering.pdf'
Run output job 'Gerber/Excellon'...
  => '{project.output_dir_native}//gbr//Empty_Project_v1_DRILLS-NPTH.drl'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_DRILLS-PTH.drl'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_OUTLINES.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_COPPER-TOP.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_COPPER-BOTTOM.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERMASK-TOP.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERMASK-BOTTOM.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_SILKSCREEN-TOP.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_SILKSCREEN-BOTTOM.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERPASTE-TOP.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERPASTE-BOTTOM.gbr'
Run output job 'Pick&Place CSV'...
  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_TOP.csv'
  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_BOT.csv'
Run output job 'Pick&Place X3'...
  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_TOP.gbr'
  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_BOT.gbr'
Run output job 'Netlist'...
  => '{project.output_dir_native}//Empty_Project_v1_Netlist.d356'
Run output job 'BOM'...
  => '{project.output_dir_native}//asm//Empty_Project_v1_BOM_AV.csv'
Run output job 'Interactive BOM'...
  => '{project.output_dir_native}//asm//Empty_Project_v1_BOM_AV.html'
Run output job 'STEP Model'...
  => '{project.output_dir_native}//Empty_Project_v1.step'
Run output job 'Custom File'...
  => '{project.output_dir_native}//Empty_Project_v1.txt'
Run output job 'ZIP'...
  => '{project.output_dir_native}//Empty_Project_v1.zip'
Run output job 'Project Data'...
  => '{project.output_dir_native}//Empty_Project_v1.json'
Run output job 'Project Archive'...
  => '{project.output_dir_native}//Empty_Project_v1.lppz'
SUCCESS
""").replace("//", os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 12


@pytest.mark.parametrize(
    "project",
    [
        params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    ],
)
def test_if_invalid_job_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run("open-project", "--run-job", "foo", project.path)
    assert stderr == "ERROR: No output job with the name 'foo' found.\n"
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Finished with errors!
""")
    assert code == 1


@pytest.mark.parametrize(
    "project",
    [
        params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    ],
)
def test_conflicting_output_file_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    # change one output file to be identical as another one
    boardfile = cli.abspath(project.dir + "/project/jobs.lp")
    for line in fileinput.input(boardfile, inplace=1):
        print(line.replace("_TOP.gbr", "_BOT.gbr"))

    code, stdout, stderr = cli.run("open-project", "--run-jobs", project.path)
    assert stderr == nofmt("""\
ERROR: Attempted to write the output file \
'asm//Empty_Project_v1_PnP_AV_BOT.gbr' multiple times! \
Make sure to specify unique output file paths, e.g. by using \
placeholders like '{{BOARD}}' or '{{VARIANT}}'.
""").replace("//", os.sep)
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Run output job 'Schematic PDF'...
  => '{project.output_dir_native}//Empty_Project_v1_Schematic.pdf'
Run output job 'Board Assembly PDF'...
  => '{project.output_dir_native}//Empty_Project_v1_Assembly.pdf'
Run output job 'Board Rendering PDF'...
  => '{project.output_dir_native}//Empty_Project_v1_Rendering.pdf'
Run output job 'Gerber/Excellon'...
  => '{project.output_dir_native}//gbr//Empty_Project_v1_DRILLS-NPTH.drl'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_DRILLS-PTH.drl'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_OUTLINES.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_COPPER-TOP.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_COPPER-BOTTOM.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERMASK-TOP.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERMASK-BOTTOM.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_SILKSCREEN-TOP.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_SILKSCREEN-BOTTOM.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERPASTE-TOP.gbr'
  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERPASTE-BOTTOM.gbr'
Run output job 'Pick&Place CSV'...
  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_TOP.csv'
  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_BOT.csv'
Run output job 'Pick&Place X3'...
  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_BOT.gbr'
  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_BOT.gbr'
Finished with errors!
""").replace("//", os.sep)
    assert code == 1


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
        params.EMPTY_PROJECT_LPPZ_PARAM,
    ],
)
def test_custom_jobs(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    jobs = """
      (librepcb_jobs
       (job a334a18d-6bf7-4e99-b48e-a26c2e2899bd (name "Custom Job")
        (type netlist)
        (board default)
        (output "custom.d356")
       )
      )
    """
    with open(cli.abspath("custom_jobs.lp"), mode="w") as f:
        f.write(jobs)
    dir = cli.abspath(project.output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project", "--run-jobs", "--jobs", "custom_jobs.lp", project.path
    )
    output_prefix = "" if project.is_lppz else (project.dir + os.sep)
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Run output job 'Custom Job'...
  => '{output_prefix}output//v1//custom.d356'
SUCCESS
""").replace("//", os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 2


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
    ],
)
def test_nonexistent_jobs_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run(
        "open-project", "--run-jobs", "--jobs=nonexistent.lp", project.path
    )
    assert stderr == nofmt(f"""\
ERROR: Failed to load custom output jobs: \
The file "{cli.abspath("nonexistent.lp")}" does not exist.
""")
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Finished with errors!
""")
    assert code == 1


@pytest.mark.parametrize(
    "project",
    [
        params.EMPTY_PROJECT_LPP_PARAM,
    ],
)
def test_invalid_jobs_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    with open(cli.abspath("custom_jobs.lp"), mode="w") as f:
        f.write("(librepcb_jobs (job))")
    code, stdout, stderr = cli.run(
        "open-project", "--run-jobs", "--jobs=custom_jobs.lp", project.path
    )
    assert stderr == nofmt("""\
ERROR: Failed to load custom output jobs: \
File parse error: Child not found: type/@0
File: \n\
Invalid Content: ''
""")
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Finished with errors!
""")
    assert code == 1


@pytest.mark.parametrize(
    "project",
    [
        params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
    ],
)
def test_custom_outdir_relative(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath("foo")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project", "--run-jobs", "--outdir=foo", project.path
    )
    if "LibrePCB was compiled without OpenCascade" in stderr:
        pytest.skip("Feature not available.")
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Run output job 'Schematic PDF'...
  => 'foo//Empty_Project_v1_Schematic.pdf'
Run output job 'Board Assembly PDF'...
  => 'foo//Empty_Project_v1_Assembly.pdf'
Run output job 'Board Rendering PDF'...
  => 'foo//Empty_Project_v1_Rendering.pdf'
Run output job 'Gerber/Excellon'...
  => 'foo//gbr//Empty_Project_v1_DRILLS-NPTH.drl'
  => 'foo//gbr//Empty_Project_v1_DRILLS-PTH.drl'
  => 'foo//gbr//Empty_Project_v1_OUTLINES.gbr'
  => 'foo//gbr//Empty_Project_v1_COPPER-TOP.gbr'
  => 'foo//gbr//Empty_Project_v1_COPPER-BOTTOM.gbr'
  => 'foo//gbr//Empty_Project_v1_SOLDERMASK-TOP.gbr'
  => 'foo//gbr//Empty_Project_v1_SOLDERMASK-BOTTOM.gbr'
  => 'foo//gbr//Empty_Project_v1_SILKSCREEN-TOP.gbr'
  => 'foo//gbr//Empty_Project_v1_SILKSCREEN-BOTTOM.gbr'
  => 'foo//gbr//Empty_Project_v1_SOLDERPASTE-TOP.gbr'
  => 'foo//gbr//Empty_Project_v1_SOLDERPASTE-BOTTOM.gbr'
Run output job 'Pick&Place CSV'...
  => 'foo//asm//Empty_Project_v1_PnP_AV_TOP.csv'
  => 'foo//asm//Empty_Project_v1_PnP_AV_BOT.csv'
Run output job 'Pick&Place X3'...
  => 'foo//asm//Empty_Project_v1_PnP_AV_TOP.gbr'
  => 'foo//asm//Empty_Project_v1_PnP_AV_BOT.gbr'
Run output job 'Netlist'...
  => 'foo//Empty_Project_v1_Netlist.d356'
Run output job 'BOM'...
  => 'foo//asm//Empty_Project_v1_BOM_AV.csv'
Run output job 'Interactive BOM'...
  => 'foo//asm//Empty_Project_v1_BOM_AV.html'
Run output job 'STEP Model'...
  => 'foo//Empty_Project_v1.step'
Run output job 'Custom File'...
  => 'foo//Empty_Project_v1.txt'
Run output job 'ZIP'...
  => 'foo//Empty_Project_v1.zip'
Run output job 'Project Data'...
  => 'foo//Empty_Project_v1.json'
Run output job 'Project Archive'...
  => 'foo//Empty_Project_v1.lppz'
SUCCESS
""").replace("//", os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 12


@pytest.mark.parametrize(
    "project",
    [
        params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
        params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
    ],
)
def test_custom_outdir_absolute(cli, project):
    """
    Note: Actually the stdout should show absolute paths as well, but that's
    not implemented correctly yet.
    """
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath("foo")
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run(
        "open-project", "--run-jobs", "--outdir", dir, project.path
    )
    if "LibrePCB was compiled without OpenCascade" in stderr:
        pytest.skip("Feature not available.")
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Run output job 'Schematic PDF'...
  => 'foo//Empty_Project_v1_Schematic.pdf'
Run output job 'Board Assembly PDF'...
  => 'foo//Empty_Project_v1_Assembly.pdf'
Run output job 'Board Rendering PDF'...
  => 'foo//Empty_Project_v1_Rendering.pdf'
Run output job 'Gerber/Excellon'...
  => 'foo//gbr//Empty_Project_v1_DRILLS-NPTH.drl'
  => 'foo//gbr//Empty_Project_v1_DRILLS-PTH.drl'
  => 'foo//gbr//Empty_Project_v1_OUTLINES.gbr'
  => 'foo//gbr//Empty_Project_v1_COPPER-TOP.gbr'
  => 'foo//gbr//Empty_Project_v1_COPPER-BOTTOM.gbr'
  => 'foo//gbr//Empty_Project_v1_SOLDERMASK-TOP.gbr'
  => 'foo//gbr//Empty_Project_v1_SOLDERMASK-BOTTOM.gbr'
  => 'foo//gbr//Empty_Project_v1_SILKSCREEN-TOP.gbr'
  => 'foo//gbr//Empty_Project_v1_SILKSCREEN-BOTTOM.gbr'
  => 'foo//gbr//Empty_Project_v1_SOLDERPASTE-TOP.gbr'
  => 'foo//gbr//Empty_Project_v1_SOLDERPASTE-BOTTOM.gbr'
Run output job 'Pick&Place CSV'...
  => 'foo//asm//Empty_Project_v1_PnP_AV_TOP.csv'
  => 'foo//asm//Empty_Project_v1_PnP_AV_BOT.csv'
Run output job 'Pick&Place X3'...
  => 'foo//asm//Empty_Project_v1_PnP_AV_TOP.gbr'
  => 'foo//asm//Empty_Project_v1_PnP_AV_BOT.gbr'
Run output job 'Netlist'...
  => 'foo//Empty_Project_v1_Netlist.d356'
Run output job 'BOM'...
  => 'foo//asm//Empty_Project_v1_BOM_AV.csv'
Run output job 'Interactive BOM'...
  => 'foo//asm//Empty_Project_v1_BOM_AV.html'
Run output job 'STEP Model'...
  => 'foo//Empty_Project_v1.step'
Run output job 'Custom File'...
  => 'foo//Empty_Project_v1.txt'
Run output job 'ZIP'...
  => 'foo//Empty_Project_v1.zip'
Run output job 'Project Data'...
  => 'foo//Empty_Project_v1.json'
Run output job 'Project Archive'...
  => 'foo//Empty_Project_v1.lppz'
SUCCESS
""").replace("//", os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 12
