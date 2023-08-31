#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pytest

"""
Test command "open-step"
"""


def test_valid_file(cli):
    fp = cli.add_file('unittests/librepcbcommon/OccModelTest/model.step')
    code, stdout, stderr = cli.run('open-step', fp)
    if 'LibrePCB was compiled without OpenCascade' in stderr:
        pytest.skip("Feature not available.")
    assert stderr == ''
    assert stdout == \
        "Open STEP file '{path}'...\n" \
        "Load model...\n" \
        "SUCCESS\n".format(path=fp)
    assert code == 0


def test_invalid_file(cli):
    fp = cli.add_file('LICENSE.txt')
    code, stdout, stderr = cli.run('open-step', fp)
    if 'LibrePCB was compiled without OpenCascade' in stderr:
        pytest.skip("Feature not available.")
    assert len(stderr) > 10
    assert "Open STEP file '{path}'...\n".format(path=fp) in stdout
    assert "Finished with errors!\n" in stdout
    assert code == 1
