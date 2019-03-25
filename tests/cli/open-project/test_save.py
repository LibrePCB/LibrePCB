#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import pytest

"""
Test command "open-project --save"
"""

PROJECT_LPP = 'data/Empty Project/Empty Project.lpp'
PROJECT_LPPZ = 'data/Empty Project.lppz'


@pytest.mark.parametrize("project", [
    PROJECT_LPP,
    PROJECT_LPPZ,
], ids=[
    'lpp',
    'lppz'
])
def test_save(cli, project):
    # append some zeros to the project file
    path = cli.abspath(project)
    with open(path, 'ab') as f:
        f.write(b'\0\0')
    original_filesize = os.path.getsize(path)
    # save project (must remove the appended zeros)
    code, stdout, stderr = cli.run('open-project', '--save', path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.getsize(path) != original_filesize
