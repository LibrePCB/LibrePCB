#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
import pytest

"""
Test command "open-project --export-pnp-top --export-pnp-bottom"
"""


@pytest.mark.parametrize("project", [params.EMPTY_PROJECT_LPP_PARAM])
@pytest.mark.parametrize("argument", [
    '--export-pnp-top=foo.bar',
    '--export-pnp-bottom=foo.bar',
])
def test_if_unknown_file_extension_fails(cli, project, argument):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project', argument, project.path)
    assert code == 1
    assert len(stderr) == 1
    assert "Unknown extension 'bar'" in stderr[0]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'


@pytest.mark.parametrize("project", [params.EMPTY_PROJECT_LPP_PARAM])
@pytest.mark.parametrize("ext", ['csv', 'gbr'])
def test_if_project_without_boards_succeeds(cli, project, ext):
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    # remove all boards first
    with open(cli.abspath(project.dir + '/boards/boards.lp'), 'w') as f:
        f.write('(librepcb_boards)')

    relpath_top = project.output_dir + 'pnp/top.' + ext
    relpath_bot = project.output_dir + 'pnp/bot.' + ext
    abspath_top = cli.abspath(relpath_top)
    abspath_bot = cli.abspath(relpath_bot)
    assert not os.path.exists(abspath_top)
    assert not os.path.exists(abspath_bot)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pnp-top=' + relpath_top,
                                   '--export-pnp-bottom=' + relpath_bot,
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert not os.path.exists(abspath_top)  # nothing exported
    assert not os.path.exists(abspath_bot)  # nothing exported


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
@pytest.mark.parametrize("ext", ['csv', 'gbr'])
def test_export_project_with_two_boards_implicit(cli, project, ext):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    fp_top = project.output_dir + '/pnp/{{BOARD}}_top.' + ext
    fp_bot = project.output_dir + '/pnp/{{BOARD}}_bot.' + ext
    dir = cli.abspath(project.output_dir + '/pnp')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pnp-top=' + fp_top,
                                   '--export-pnp-bottom=' + fp_bot,
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 4


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
@pytest.mark.parametrize("ext", ['csv', 'gbr'])
def test_export_project_with_two_boards_explicit_one(cli, project, ext):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    fp_top = project.output_dir + '/pnp/{{BOARD}}_top.' + ext
    fp_bot = project.output_dir + '/pnp/{{BOARD}}_bot.' + ext
    dir = cli.abspath(project.output_dir + '/pnp')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pnp-top=' + fp_top,
                                   '--export-pnp-bottom=' + fp_bot,
                                   '--board=copy',
                                   project.path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert len(os.listdir(dir)) == 2


@pytest.mark.parametrize("project", [params.PROJECT_WITH_TWO_BOARDS_LPP])
@pytest.mark.parametrize("ext", ['csv', 'gbr'])
def test_export_project_with_two_conflicting_boards_fails(cli, project, ext):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    fp_top = project.output_dir + '/top.' + ext
    fp_bot = project.output_dir + '/bot.' + ext
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pnp-top=' + fp_top,
                                   '--export-pnp-bottom=' + fp_bot,
                                   project.path)
    assert code == 1
    assert len(stderr) > 0
    assert 'was written multiple times' in stderr[0]
    assert 'NOTE: To avoid writing files multiple times,' in stderr[-1]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'
