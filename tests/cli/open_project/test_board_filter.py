#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
import pytest

"""
Test command "open-project --board --board-index"
"""


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
])
def test_no_filter(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom={{BOARD}}.csv',
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export board-specific BOM to '{{{{BOARD}}}}.csv'...\n" \
        "  - 'default' => 'default.csv'\n" \
        "  - 'copy' => 'copy.csv'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
])
def test_filter_by_name(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom={{BOARD}}.csv',
                                   '--board=copy',
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export board-specific BOM to '{{{{BOARD}}}}.csv'...\n" \
        "  - 'copy' => 'copy.csv'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
])
def test_filter_by_names(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom={{BOARD}}.csv',
                                   '--board=copy',  # --arg=<value>
                                   '--board', 'default',  # --arg <value>
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export board-specific BOM to '{{{{BOARD}}}}.csv'...\n" \
        "  - 'copy' => 'copy.csv'\n" \
        "  - 'default' => 'default.csv'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
])
def test_filter_by_invalid_name(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom={{BOARD}}.csv',
                                   '--board=foo',
                                   project.path)
    assert stderr == "ERROR: No board with the name 'foo' found.\n"
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export board-specific BOM to '{{{{BOARD}}}}.csv'...\n" \
        "Finished with errors!\n".format(project=project).replace('//', os.sep)
    assert code == 1


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
])
def test_filter_by_index(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom={{BOARD}}.csv',
                                   '--board-index=1',
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export board-specific BOM to '{{{{BOARD}}}}.csv'...\n" \
        "  - 'copy' => 'copy.csv'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
])
def test_filter_by_indices(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom={{BOARD}}.csv',
                                   '--board-index=1',  # --arg=<value>
                                   '--board-index', '0',  # --arg <value>
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export board-specific BOM to '{{{{BOARD}}}}.csv'...\n" \
        "  - 'copy' => 'copy.csv'\n" \
        "  - 'default' => 'default.csv'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
])
def test_filter_by_invalid_index(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom={{BOARD}}.csv',
                                   '--board-index=5',
                                   '--board-index=foo',
                                   project.path)
    assert stderr == \
        "ERROR: Board index '5' is invalid.\n" \
        "ERROR: Board index 'foo' is invalid.\n"
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export board-specific BOM to '{{{{BOARD}}}}.csv'...\n" \
        "Finished with errors!\n".format(project=project).replace('//', os.sep)
    assert code == 1


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
])
def test_filter_by_name_and_index_same(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom={{BOARD}}.csv',
                                   '--board=copy',
                                   '--board-index=1',
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export board-specific BOM to '{{{{BOARD}}}}.csv'...\n" \
        "  - 'copy' => 'copy.csv'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
])
def test_filter_by_name_and_index_different(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-board-bom={{BOARD}}.csv',
                                   '--board=default',
                                   '--board-index=1',
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export board-specific BOM to '{{{{BOARD}}}}.csv'...\n" \
        "  - 'default' => 'default.csv'\n" \
        "  - 'copy' => 'copy.csv'\n" \
        "SUCCESS\n".format(project=project).replace('//', os.sep)
    assert code == 0
