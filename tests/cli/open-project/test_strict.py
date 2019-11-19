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
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'


def test_invalid_lpp(cli):
    project = params.EMPTY_PROJECT_LPP
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    # append some zeros to the project file
    path = cli.abspath(project.path)
    with open(path, 'ab') as f:
        f.write(b'\0\0')
    # open project
    code, stdout, stderr = cli.run('open-project', '--strict', project.path)
    assert code == 1
    assert len(stderr) == 1
    assert 'Non-canonical file:' in stderr[0]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'


def test_lppz_fails(cli):
    project = params.PROJECT_WITH_TWO_BOARDS_LPPZ
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project', '--strict', project.path)
    assert code == 1
    assert len(stderr) == 1
    assert "The option '--strict' is not available" in stderr[0]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'
