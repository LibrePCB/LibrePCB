#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import fileinput
import params
import pytest

"""
Test command "open-project --run-jobs"
"""


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
])
def test_if_project_without_jobs_succeeds(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    dir = cli.abspath(project.output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--run-jobs',
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "SUCCESS\n".format(project=project)
    assert code == 0
    assert os.path.exists(dir)
    assert os.listdir(dir) == ['.librepcb-output']


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_project_with_jobs(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath(project.output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--run-jobs',
                                   project.path)
    if 'LibrePCB was compiled without OpenCascade' in stderr:
        pytest.skip("Feature not available.")
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Run output job 'Schematic PDF'...\n" \
        "  => '{project.output_dir_native}//Empty_Project_v1_Schematic.pdf'\n" \
        "Run output job 'Board Assembly PDF'...\n" \
        "  => '{project.output_dir_native}//Empty_Project_v1_Assembly.pdf'\n" \
        "Run output job 'Gerber/Excellon'...\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_DRILLS-NPTH.drl'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_DRILLS-PTH.drl'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_OUTLINES.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_COPPER-TOP.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_COPPER-BOTTOM.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERMASK-TOP.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERMASK-BOTTOM.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_SILKSCREEN-TOP.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_SILKSCREEN-BOTTOM.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERPASTE-TOP.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERPASTE-BOTTOM.gbr'\n" \
        "Run output job 'Pick&Place CSV'...\n" \
        "  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_TOP.csv'\n" \
        "  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_BOT.csv'\n" \
        "Run output job 'Pick&Place X3'...\n" \
        "  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_TOP.gbr'\n" \
        "  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_BOT.gbr'\n" \
        "Run output job 'Netlist'...\n" \
        "  => '{project.output_dir_native}//Empty_Project_v1_Netlist.d356'\n" \
        "Run output job 'BOM'...\n" \
        "  => '{project.output_dir_native}//asm//Empty_Project_v1_BOM_AV.csv'\n" \
        "Run output job 'STEP Model'...\n" \
        "  => '{project.output_dir_native}//Empty_Project_v1.step'\n" \
        "Run output job 'Custom File'...\n" \
        "  => '{project.output_dir_native}//Empty_Project_v1.txt'\n" \
        "Run output job 'ZIP'...\n" \
        "  => '{project.output_dir_native}//Empty_Project_v1.zip'\n" \
        "Run output job 'Project Data'...\n" \
        "  => '{project.output_dir_native}//Empty_Project_v1.json'\n" \
        "Run output job 'Project Archive'...\n" \
        "  => '{project.output_dir_native}//Empty_Project_v1.lppz'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 11


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
])
def test_if_invalid_job_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project',
                                   '--run-job', 'foo',
                                   project.path)
    assert stderr == "ERROR: No output job with the name 'foo' found.\n"
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Finished with errors!\n".format(project=project)
    assert code == 1


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
])
def test_conflicting_output_file_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    # change one output file to be identical as another one
    boardfile = cli.abspath(project.dir + '/project/jobs.lp')
    for line in fileinput.input(boardfile, inplace=1):
        print(line.replace("_TOP.gbr", "_BOT.gbr"))

    code, stdout, stderr = cli.run('open-project',
                                   '--run-jobs',
                                   project.path)
    assert stderr == \
        "ERROR: Attempted to write the output file " \
        "'asm//Empty_Project_v1_PnP_AV_BOT.gbr' multiple times! " \
        "Make sure to specify unique output file paths, e.g. by using " \
        "placeholders like '{{BOARD}}' or '{{VARIANT}}'.\n" \
        .replace('//', os.sep)
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Run output job 'Schematic PDF'...\n" \
        "  => '{project.output_dir_native}//Empty_Project_v1_Schematic.pdf'\n" \
        "Run output job 'Board Assembly PDF'...\n" \
        "  => '{project.output_dir_native}//Empty_Project_v1_Assembly.pdf'\n" \
        "Run output job 'Gerber/Excellon'...\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_DRILLS-NPTH.drl'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_DRILLS-PTH.drl'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_OUTLINES.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_COPPER-TOP.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_COPPER-BOTTOM.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERMASK-TOP.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERMASK-BOTTOM.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_SILKSCREEN-TOP.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_SILKSCREEN-BOTTOM.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERPASTE-TOP.gbr'\n" \
        "  => '{project.output_dir_native}//gbr//Empty_Project_v1_SOLDERPASTE-BOTTOM.gbr'\n" \
        "Run output job 'Pick&Place CSV'...\n" \
        "  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_TOP.csv'\n" \
        "  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_BOT.csv'\n" \
        "Run output job 'Pick&Place X3'...\n" \
        "  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_BOT.gbr'\n" \
        "  => '{project.output_dir_native}//asm//Empty_Project_v1_PnP_AV_BOT.gbr'\n" \
        "Finished with errors!\n".format(project=project).replace('//', os.sep)
    assert code == 1


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.EMPTY_PROJECT_LPPZ_PARAM,
])
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
    with open(cli.abspath('custom_jobs.lp'), mode='w') as f:
        f.write(jobs)
    dir = cli.abspath(project.output_dir)
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--run-jobs',
                                   '--jobs', 'custom_jobs.lp',
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Run output job 'Custom Job'...\n" \
        "  => '{output_prefix}output//v1//custom.d356'\n" \
        "SUCCESS\n".format(
            project=project,
            output_prefix=('' if project.is_lppz else (project.dir + os.sep)),
        ).replace('//', os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 2


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
])
def test_nonexistent_jobs_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project',
                                   '--run-jobs',
                                   '--jobs=nonexistent.lp',
                                   project.path)
    assert stderr == \
        "ERROR: Failed to load custom output jobs: " \
        "The file \"{file}\" does not exist.\n" \
        .format(file=cli.abspath('nonexistent.lp'))
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Finished with errors!\n".format(project=project)
    assert code == 1


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
])
def test_invalid_jobs_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    with open(cli.abspath('custom_jobs.lp'), mode='w') as f:
        f.write('(librepcb_jobs (job))')
    code, stdout, stderr = cli.run('open-project',
                                   '--run-jobs',
                                   '--jobs=custom_jobs.lp',
                                   project.path)
    assert stderr == \
        "ERROR: Failed to load custom output jobs: File parse error: " \
        "Child not found: type/@0\n\n" \
        "File: \n" \
        "Line,Column: -1,-1\n" \
        "Invalid Content: \"\"\n"
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Finished with errors!\n".format(project=project)
    assert code == 1


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_custom_outdir_relative(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath('foo')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--run-jobs',
                                   '--outdir=foo',
                                   project.path)
    if 'LibrePCB was compiled without OpenCascade' in stderr:
        pytest.skip("Feature not available.")
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Run output job 'Schematic PDF'...\n" \
        "  => 'foo//Empty_Project_v1_Schematic.pdf'\n" \
        "Run output job 'Board Assembly PDF'...\n" \
        "  => 'foo//Empty_Project_v1_Assembly.pdf'\n" \
        "Run output job 'Gerber/Excellon'...\n" \
        "  => 'foo//gbr//Empty_Project_v1_DRILLS-NPTH.drl'\n" \
        "  => 'foo//gbr//Empty_Project_v1_DRILLS-PTH.drl'\n" \
        "  => 'foo//gbr//Empty_Project_v1_OUTLINES.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_COPPER-TOP.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_COPPER-BOTTOM.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_SOLDERMASK-TOP.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_SOLDERMASK-BOTTOM.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_SILKSCREEN-TOP.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_SILKSCREEN-BOTTOM.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_SOLDERPASTE-TOP.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_SOLDERPASTE-BOTTOM.gbr'\n" \
        "Run output job 'Pick&Place CSV'...\n" \
        "  => 'foo//asm//Empty_Project_v1_PnP_AV_TOP.csv'\n" \
        "  => 'foo//asm//Empty_Project_v1_PnP_AV_BOT.csv'\n" \
        "Run output job 'Pick&Place X3'...\n" \
        "  => 'foo//asm//Empty_Project_v1_PnP_AV_TOP.gbr'\n" \
        "  => 'foo//asm//Empty_Project_v1_PnP_AV_BOT.gbr'\n" \
        "Run output job 'Netlist'...\n" \
        "  => 'foo//Empty_Project_v1_Netlist.d356'\n" \
        "Run output job 'BOM'...\n" \
        "  => 'foo//asm//Empty_Project_v1_BOM_AV.csv'\n" \
        "Run output job 'STEP Model'...\n" \
        "  => 'foo//Empty_Project_v1.step'\n" \
        "Run output job 'Custom File'...\n" \
        "  => 'foo//Empty_Project_v1.txt'\n" \
        "Run output job 'ZIP'...\n" \
        "  => 'foo//Empty_Project_v1.zip'\n" \
        "Run output job 'Project Data'...\n" \
        "  => 'foo//Empty_Project_v1.json'\n" \
        "Run output job 'Project Archive'...\n" \
        "  => 'foo//Empty_Project_v1.lppz'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 11


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_custom_outdir_absolute(cli, project):
    """
    Note: Actually the stdout should show absolute paths as well, but that's
    not implemented correctly yet.
    """
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath('foo')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--run-jobs',
                                   '--outdir', dir,
                                   project.path)
    if 'LibrePCB was compiled without OpenCascade' in stderr:
        pytest.skip("Feature not available.")
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Run output job 'Schematic PDF'...\n" \
        "  => 'foo//Empty_Project_v1_Schematic.pdf'\n" \
        "Run output job 'Board Assembly PDF'...\n" \
        "  => 'foo//Empty_Project_v1_Assembly.pdf'\n" \
        "Run output job 'Gerber/Excellon'...\n" \
        "  => 'foo//gbr//Empty_Project_v1_DRILLS-NPTH.drl'\n" \
        "  => 'foo//gbr//Empty_Project_v1_DRILLS-PTH.drl'\n" \
        "  => 'foo//gbr//Empty_Project_v1_OUTLINES.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_COPPER-TOP.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_COPPER-BOTTOM.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_SOLDERMASK-TOP.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_SOLDERMASK-BOTTOM.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_SILKSCREEN-TOP.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_SILKSCREEN-BOTTOM.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_SOLDERPASTE-TOP.gbr'\n" \
        "  => 'foo//gbr//Empty_Project_v1_SOLDERPASTE-BOTTOM.gbr'\n" \
        "Run output job 'Pick&Place CSV'...\n" \
        "  => 'foo//asm//Empty_Project_v1_PnP_AV_TOP.csv'\n" \
        "  => 'foo//asm//Empty_Project_v1_PnP_AV_BOT.csv'\n" \
        "Run output job 'Pick&Place X3'...\n" \
        "  => 'foo//asm//Empty_Project_v1_PnP_AV_TOP.gbr'\n" \
        "  => 'foo//asm//Empty_Project_v1_PnP_AV_BOT.gbr'\n" \
        "Run output job 'Netlist'...\n" \
        "  => 'foo//Empty_Project_v1_Netlist.d356'\n" \
        "Run output job 'BOM'...\n" \
        "  => 'foo//asm//Empty_Project_v1_BOM_AV.csv'\n" \
        "Run output job 'STEP Model'...\n" \
        "  => 'foo//Empty_Project_v1.step'\n" \
        "Run output job 'Custom File'...\n" \
        "  => 'foo//Empty_Project_v1.txt'\n" \
        "Run output job 'ZIP'...\n" \
        "  => 'foo//Empty_Project_v1.zip'\n" \
        "Run output job 'Project Data'...\n" \
        "  => 'foo//Empty_Project_v1.json'\n" \
        "Run output job 'Project Archive'...\n" \
        "  => 'foo//Empty_Project_v1.lppz'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 11
