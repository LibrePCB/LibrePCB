#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pytest

"""
Test command "open-project"
"""

PROJECT_LPP = 'data/Empty Project/Empty Project.lpp'
PROJECT_LPPZ = 'data/Empty Project.lppz'


def test_help(cli):
    code, stdout, stderr = cli.run('open-project', '--help')
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 20


@pytest.mark.parametrize("project", [
    PROJECT_LPP,
    PROJECT_LPPZ,
])
def test_open_project_absolute_path(cli, project):
    code, stdout, stderr = cli.run('open-project', cli.abspath(project))
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'


@pytest.mark.parametrize("project", [
    PROJECT_LPP,
    PROJECT_LPPZ,
], ids=[
    'lpp',
    'lppz'
])
def test_open_project_relative_path(cli, project):
    code, stdout, stderr = cli.run('open-project', project)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'


@pytest.mark.parametrize("project", [
    PROJECT_LPP,
    PROJECT_LPPZ,
])
def test_open_project_verbose(cli, project):
    code, stdout, stderr = cli.run('open-project', '--verbose', project)
    assert code == 0
    assert len(stderr) > 0  # logging messages are on stderr
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
