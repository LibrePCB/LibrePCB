#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import pytest

"""
Test command "open-project --export-schematics"
"""

PROJECT_DIR_1 = 'data/Empty Project'
PROJECT_PATH_1 = PROJECT_DIR_1 + '/Empty Project.lpp'

PROJECT_DIR_2 = 'data/Project With Two Boards'
PROJECT_PATH_2 = PROJECT_DIR_2 + '.lppz'


@pytest.mark.parametrize("project", [
    PROJECT_PATH_1,
    PROJECT_PATH_2,
], ids=[
    'EmptyProject.lpp',
    'ProjectWithTwoBoards.lppz',
])
def test_if_unknown_file_extension_fails(cli, project):
    code, stdout, stderr = cli.run('open-project',
                                   '--export-schematics=foo.bar',
                                   project)
    assert code == 1
    assert len(stderr) == 1
    assert 'Unknown extension' in stderr[0]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'


@pytest.mark.parametrize("project", [
    PROJECT_PATH_1,
    PROJECT_PATH_2,
], ids=[
    'EmptyProject.lpp',
    'ProjectWithTwoBoards.lppz',
])
def test_exporting_pdf_with_relative_path(cli, project):
    path = cli.abspath('sch.pdf')
    assert not os.path.exists(path)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-schematics=sch.pdf',
                                   project)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(path)


@pytest.mark.parametrize("project", [
    PROJECT_PATH_1,
    PROJECT_PATH_2,
], ids=[
    'EmptyProject.lpp',
    'ProjectWithTwoBoards.lppz',
])
def test_exporting_pdf_with_absolute_path(cli, project):
    path = cli.abspath('schematic with spaces.pdf')
    assert not os.path.exists(path)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-schematics={}'.format(path),
                                   project)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(path)


@pytest.mark.parametrize("project", [
    PROJECT_PATH_1,
    PROJECT_PATH_2,
], ids=[
    'EmptyProject.lpp',
    'ProjectWithTwoBoards.lppz',
])
def test_if_output_directories_are_created(cli, project):
    dir = cli.abspath('nonexistent directory/nested')
    path = os.path.join(dir, 'schematic.pdf')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-schematics={}'.format(path),
                                   project)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert os.path.exists(path)
