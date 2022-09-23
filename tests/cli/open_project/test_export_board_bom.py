#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
import pytest

"""
Test command "open-project --export-board-bom"
"""


@pytest.mark.parametrize("project", [params.EMPTY_PROJECT_LPP_PARAM])
def test_if_project_without_boards_succeeds(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    # remove all boards first
    with open(cli.abspath(project.dir + '/boards/boards.lp'), 'w') as f:
        f.write('(librepcb_boards)')

    relpath = project.output_dir + '/bom/bom.csv'
    abspath = cli.abspath(relpath)
    assert not os.path.exists(abspath)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom=' + relpath,
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export board-specific BOM to 'Empty Project/output/v1/bom/bom.csv'...\n" \
        "SUCCESS\n".format(project=project)
    assert code == 0
    assert not os.path.exists(abspath)  # nothing exported


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_export_project_with_two_boards_implicit(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    fp = project.output_dir + '/bom/{{BOARD}}.csv'
    dir = cli.abspath(project.output_dir + '/bom')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom=' + fp,
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export board-specific BOM to '{project.output_dir}/bom/{{{{BOARD}}}}.csv'...\n" \
        "  - 'default' => '{project.output_dir_native}//bom//default.csv'\n" \
        "  - 'copy' => '{project.output_dir_native}//bom//copy.csv'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 2


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_export_project_with_two_boards_explicit_one_attributes(cli, project):
    """
    Note: Test with passing the argument as "--arg <value>".
    """
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    fp = project.output_dir + '/bom/{{BOARD}}.csv'
    dir = cli.abspath(project.output_dir + '/bom')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom', fp,
                                   '--board=copy',
                                   '--bom-attributes', 'MANUFACTURER, MPN',
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export board-specific BOM to '{project.output_dir}/bom/{{{{BOARD}}}}.csv'...\n" \
        "  - 'copy' => '{project.output_dir_native}//bom//copy.csv'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0
    assert os.path.exists(dir)
    assert os.listdir(dir) == ['copy.csv']
    fp = cli.abspath(fp.replace('{{BOARD}}', 'copy'))
    assert b'MANUFACTURER,MPN' in open(fp, 'rb').read()


@pytest.mark.parametrize("project", [params.PROJECT_WITH_TWO_BOARDS_LPP])
def test_export_project_with_two_conflicting_boards_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    fp = project.output_dir + '/bom.csv'
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom=' + fp,
                                   project.path)
    assert stderr == \
        "ERROR: The file '{project.output_dir_native}//bom.csv' was " \
        "written multiple times!\n" \
        "NOTE: To avoid writing files multiple times, make sure to pass " \
        "unique filepaths to all export functions. For board output files, " \
        "you could either add the placeholder '{{{{BOARD}}}}' to the path " \
        "or specify the boards to export with the '--board' argument.\n" \
        .format(project=project).replace('//', os.sep)
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export board-specific BOM to '{project.output_dir}/bom.csv'...\n" \
        "  - 'default' => '{project.output_dir_native}//bom.csv'\n" \
        "  - 'copy' => '{project.output_dir_native}//bom.csv'\n" \
        "Finished with errors!\n".format(project=project).replace('//', os.sep)
    assert code == 1
