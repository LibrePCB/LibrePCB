#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
import pytest

"""
Test command "open-project --export-schematics"
"""


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_if_unknown_file_extension_fails(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-schematics=foo.bar',
                                   project.path)
    assert code == 1
    assert len(stderr) == 1
    assert 'Failed to export image' in stderr[0]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_exporting_pdf_with_relative_path(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    path = cli.abspath('sch.pdf')
    assert not os.path.exists(path)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-schematics=sch.pdf',
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(path)


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_exporting_pdf_with_absolute_path(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    path = cli.abspath('schematic with spaces.pdf')
    assert not os.path.exists(path)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-schematics={}'.format(path),
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(path)


@pytest.mark.parametrize("file_extension", [
    'svg',
    'png',
])
def test_exporting_images(cli, file_extension):
    project = params.PROJECT_WITH_TWO_BOARDS_LPPZ
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    path = cli.abspath('schematic with spaces.' + file_extension)
    assert not os.path.exists(path)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-schematics={}'.format(path),
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(path)


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_if_output_directories_are_created(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    dir = cli.abspath('nonexistent directory/nested')
    path = os.path.join(dir, 'schematic.pdf')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-schematics={}'.format(path),
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert os.path.exists(path)
