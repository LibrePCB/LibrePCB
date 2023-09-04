#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pytest

"""
Test command "open-step --minify"
"""


def test_valid_file(cli):
    fp = cli.add_file('unittests/librepcbcommon/OccModelTest/model.step')
    code, stdout, stderr = cli.run('open-step', '--minify', fp)
    if 'LibrePCB was compiled without OpenCascade' in stderr:
        pytest.skip("Feature not available.")
    assert stderr == ''
    assert stdout == \
        "Open STEP file '{path}'...\n" \
        "Perform minify...\n" \
        " - Minified from 28,521 bytes to 11,716 bytes (-59%)\n" \
        "Load model...\n" \
        "SUCCESS\n".format(path=fp)
    assert code == 0


def test_invalid_file(cli):
    fp = cli.add_file('LICENSE.txt')
    code, stdout, stderr = cli.run('open-step', '--minify', fp)
    if 'LibrePCB was compiled without OpenCascade' in stderr:
        pytest.skip("Feature not available.")
    assert stderr == 'ERROR: STEP data section not found.\n'
    assert stdout == \
        "Open STEP file '{path}'...\n" \
        "Perform minify...\n" \
        "Finished with errors!\n".format(path=fp)
    assert code == 1


def test_on_save_to_output(cli):
    fp = cli.add_file('unittests/librepcbcommon/OccModelTest/model.step')
    fp_out = cli.abspath('out.step')
    code, stdout, stderr = cli.run('open-step', '--minify',
                                   '--save-to', fp_out, fp)
    if 'LibrePCB was compiled without OpenCascade' in stderr:
        pytest.skip("Feature not available.")
    assert stderr == ''
    assert stdout == \
        "Open STEP file '{path}'...\n" \
        "Perform minify...\n" \
        " - Minified from 28,521 bytes to 11,716 bytes (-59%)\n" \
        "Save to '{outpath}'...\n" \
        "Load model...\n" \
        "SUCCESS\n".format(path=fp, outpath=fp_out)
    assert code == 0

    code, stdout, stderr = cli.run('open-step', '--minify', fp_out)
    assert stderr == ''
    assert stdout == \
        "Open STEP file '{path}'...\n" \
        "Perform minify...\n" \
        " - File is already minified\n" \
        "Load model...\n" \
        "SUCCESS\n".format(path=fp_out)
    assert code == 0
