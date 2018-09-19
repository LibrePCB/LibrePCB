#!/usr/bin/env python
# -*- coding: utf-8 -*-

import re

"""
Test "--help"
"""


def test_explicit(cli):
    code, stdout, stderr = cli.run('--help')
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 10
    assert re.match('Usage: .+', stdout[0])


def test_implicit_if_no_arguments(cli):
    code, stdout, stderr = cli.run()
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 10
    assert re.match('Usage: .+', stdout[0])


def test_implicit_if_passing_invalid_argument(cli):
    code, stdout, stderr = cli.run('--invalid_argument')
    assert code == 1
    assert len(stderr) > 0
    assert re.match('Unknown option .+', stderr[0])
    assert len(stdout) > 5
    assert re.match('Usage: .+', stdout[0])
