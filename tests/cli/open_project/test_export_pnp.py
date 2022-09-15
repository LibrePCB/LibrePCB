#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
import pytest

"""
Test command "open-project --export-pnp-top --export-pnp-bottom"
"""


@pytest.mark.parametrize("project", [params.EMPTY_PROJECT_LPP_PARAM])
@pytest.mark.parametrize("argument,side", [
    ('--export-pnp-top=foo.bar', 'top'),
    ('--export-pnp-bottom=foo.bar', 'bottom'),
])
def test_if_unknown_file_extension_fails(cli, project, argument, side):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project', argument, project.path)
    assert stderr == "  ERROR: Unknown extension 'bar'.\n"
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export {side} assembly data to 'foo.bar'...\n" \
        "  - 'default' => 'foo.bar'\n" \
        "Finished with errors!\n" \
        .format(project=project, side=side)
    assert code == 1


@pytest.mark.parametrize("project", [params.EMPTY_PROJECT_LPP_PARAM])
@pytest.mark.parametrize("ext", ['csv', 'gbr'])
def test_if_project_without_boards_succeeds(cli, project, ext):
    cli.add_project(project.dir, as_lppz=project.is_lppz)

    # remove all boards first
    with open(cli.abspath(project.dir + '/boards/boards.lp'), 'w') as f:
        f.write('(librepcb_boards)')

    relpath_top = project.output_dir + '/pnp/top.' + ext
    relpath_bot = project.output_dir + '/pnp/bot.' + ext
    abspath_top = cli.abspath(relpath_top)
    abspath_bot = cli.abspath(relpath_bot)
    assert not os.path.exists(abspath_top)
    assert not os.path.exists(abspath_bot)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-pnp-top=' + relpath_top,
                                   '--export-pnp-bottom=' + relpath_bot,
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export top assembly data to '{project.output_dir}/pnp/top.{ext}'...\n" \
        "Export bottom assembly data to '{project.output_dir}/pnp/bot.{ext}'...\n" \
        "SUCCESS\n".format(project=project, ext=ext)
    assert code == 0
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
                                   '--export-pnp-top=' + fp_top,  # --arg="val"
                                   '--export-pnp-bottom', fp_bot,  # --arg "val"
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export top assembly data to '{project.output_dir}/pnp/{{{{BOARD}}}}_top.{ext}'...\n" \
        "  - 'default' => '{project.output_dir_native}//pnp//default_top.{ext}'\n" \
        "  - 'copy' => '{project.output_dir_native}//pnp//copy_top.{ext}'\n" \
        "Export bottom assembly data to '{project.output_dir}/pnp/{{{{BOARD}}}}_bot.{ext}'...\n" \
        "  - 'default' => '{project.output_dir_native}//pnp//default_bot.{ext}'\n" \
        "  - 'copy' => '{project.output_dir_native}//pnp//copy_bot.{ext}'\n" \
        "SUCCESS\n".format(project=project, ext=ext).replace('//', os.sep)
    assert code == 0
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
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export top assembly data to '{project.output_dir}/pnp/{{{{BOARD}}}}_top.{ext}'...\n" \
        "  - 'copy' => '{project.output_dir_native}//pnp//copy_top.{ext}'\n" \
        "Export bottom assembly data to '{project.output_dir}/pnp/{{{{BOARD}}}}_bot.{ext}'...\n" \
        "  - 'copy' => '{project.output_dir_native}//pnp//copy_bot.{ext}'\n" \
        "SUCCESS\n".format(project=project, ext=ext).replace('//', os.sep)
    assert code == 0
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
    assert stderr == \
        "ERROR: The file '{project.output_dir_native}//bot.{ext}' was " \
        "written multiple times!\n" \
        "ERROR: The file '{project.output_dir_native}//top.{ext}' was " \
        "written multiple times!\n" \
        "NOTE: To avoid writing files multiple times, make sure to pass " \
        "unique filepaths to all export functions. For board output files, " \
        "you could either add the placeholder '{{{{BOARD}}}}' to the path or " \
        "specify the boards to export with the '--board' argument.\n" \
        .format(project=project, ext=ext).replace('//', os.sep)
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Export top assembly data to '{project.output_dir}/top.{ext}'...\n" \
        "  - 'default' => '{project.output_dir_native}//top.{ext}'\n" \
        "  - 'copy' => '{project.output_dir_native}//top.{ext}'\n" \
        "Export bottom assembly data to '{project.output_dir}/bot.{ext}'...\n" \
        "  - 'default' => '{project.output_dir_native}//bot.{ext}'\n" \
        "  - 'copy' => '{project.output_dir_native}//bot.{ext}'\n" \
        "Finished with errors!\n" \
        .format(project=project, ext=ext).replace('//', os.sep)
    assert code == 1
