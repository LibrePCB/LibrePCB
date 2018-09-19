#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test command "open-project"
"""

PROJECT_DIR = 'data/Empty Project/'
PROJECT_PATH = PROJECT_DIR + 'Empty Project.lpp'


def test_help(cli):
    code, stdout, stderr = cli.run('open-project', '--help')
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 20


def test_open_project_absolute_path(cli):
    code, stdout, stderr = cli.run('open-project', cli.abspath(PROJECT_PATH))
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'


def test_open_project_relative_path(cli):
    code, stdout, stderr = cli.run('open-project', PROJECT_PATH)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'


def test_open_project_verbose(cli):
    code, stdout, stderr = cli.run('open-project', '--verbose', PROJECT_PATH)
    assert code == 0
    assert len(stderr) > 0  # logging messages are on stderr
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
