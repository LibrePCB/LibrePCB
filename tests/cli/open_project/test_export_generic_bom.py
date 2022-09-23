#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
import pytest

"""
Test command "open-project --export-bom"
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
                                   '--export-bom=' + relpath,
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export generic BOM to '{project.output_dir}/bom/bom.csv'...\n" \
        "  => '{project.output_dir_native}//bom//bom.csv'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0
    assert os.path.exists(abspath)


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_export_multiple_files(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    relpath1 = project.output_dir + '/bom1.csv'
    abspath1 = cli.abspath(relpath1)
    assert not os.path.exists(abspath1)
    relpath2 = project.output_dir + '/bom2.csv'
    abspath2 = cli.abspath(relpath2)
    assert not os.path.exists(abspath2)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-bom=' + relpath1,  # --arg="value"
                                   '--export-bom', relpath2,  # --arg "value"
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export generic BOM to '{project.output_dir}/bom1.csv'...\n" \
        "  => '{project.output_dir_native}//bom1.csv'\n" \
        "Export generic BOM to '{project.output_dir}/bom2.csv'...\n" \
        "  => '{project.output_dir_native}//bom2.csv'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0
    assert os.path.exists(abspath1)
    assert os.path.exists(abspath2)
