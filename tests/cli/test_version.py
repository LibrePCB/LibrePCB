#!/usr/bin/env python
# -*- coding: utf-8 -*-

import re

"""
Test "--version"
"""


def test_valid_call(cli):
    code, stdout, stderr = cli.run('--version')
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) == 5
    assert re.match('LibrePCB CLI Version \\d+\\.\\d+\\.\\d+(\\-\\w+)?$', stdout[0])
    assert re.match('File Format .+', stdout[1])
    assert re.match('Git Revision .+', stdout[2])
    assert re.match('Qt Version .+', stdout[3])
    assert re.match('Built at .+', stdout[4])
