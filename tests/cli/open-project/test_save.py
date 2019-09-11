#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
import pytest

"""
Test command "open-project --save"
"""

PROJECT_LPP = 'data/Empty Project/Empty Project.lpp'
PROJECT_LPPZ = 'data/Empty Project.lppz'


@pytest.mark.parametrize("project", [
    params.EMPTY_PROJECT_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_save(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    # append some zeros to the project file
    path = cli.abspath(project.path)
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
