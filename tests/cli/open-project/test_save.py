#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

"""
Test command "open-project --save"
"""

PROJECT_DIR = 'data/Empty Project/'
PROJECT_PATH = PROJECT_DIR + 'Empty Project.lpp'


def test_save(cli):
    # append some linebreaks to the project file
    path = cli.abspath(PROJECT_PATH)
    original_filesize = os.path.getsize(path)
    with open(path, 'ab') as f:
        f.write(b'\n\n')
    assert os.path.getsize(path) == original_filesize + 2
    # save project (must remove the appended newlines)
    code, stdout, stderr = cli.run('open-project', '--save', path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.getsize(path) == original_filesize
