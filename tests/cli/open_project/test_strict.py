#!/usr/bin/env python
# -*- coding: utf-8 -*-

import params

"""
Test command "open-project --strict"
"""


def test_valid_lpp(cli):
    project = params.EMPTY_PROJECT_LPP
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project', '--strict', project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Check for non-canonical files...\n" \
        "SUCCESS\n".format(project=project)
    assert code == 0


def test_invalid_lpp(cli):
    project = params.EMPTY_PROJECT_LPP
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    # append some zeros to the project file
    path = cli.abspath(project.path)
    with open(path, 'ab') as f:
        f.write(b'\0\0')
    # open project
    code, stdout, stderr = cli.run('open-project', '--strict', project.path)
    assert stderr == \
        "    - Non-canonical file: '{project.path}'\n" \
        .format(project=project)
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Check for non-canonical files...\n" \
        "Finished with errors!\n".format(project=project)
    assert code == 1


def test_lppz_fails(cli):
    project = params.PROJECT_WITH_TWO_BOARDS_LPPZ
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project', '--strict', project.path)
    assert stderr == \
        "  ERROR: The option '--strict' is not available for *.lppz files!\n"
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Check for non-canonical files...\n" \
        "Finished with errors!\n".format(project=project)
    assert code == 1
