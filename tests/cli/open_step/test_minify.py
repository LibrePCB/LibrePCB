#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pytest
from helpers import nofmt

"""
Test command "open-step --minify"
"""


def test_valid_file(cli):
    fp = cli.add_file("unittests/librepcbcommon/OccModelTest/model.step")
    code, stdout, stderr = cli.run("open-step", "--minify", fp)
    if "LibrePCB was compiled without OpenCascade" in stderr:
        pytest.skip("Feature not available.")
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open STEP file '{fp}'...
Perform minify...
 - Minified from 28,521 bytes to 18,336 bytes (-36%)
Load model...
SUCCESS
""")
    assert code == 0


def test_invalid_file(cli):
    fp = cli.add_file("LICENSE.txt")
    code, stdout, stderr = cli.run("open-step", "--minify", fp)
    if "LibrePCB was compiled without OpenCascade" in stderr:
        pytest.skip("Feature not available.")
    assert stderr == "ERROR: STEP data section not found.\n"
    assert stdout == nofmt(f"""\
Open STEP file '{fp}'...
Perform minify...
Finished with errors!
""")
    assert code == 1


def test_on_save_to_output(cli):
    fp = cli.add_file("unittests/librepcbcommon/OccModelTest/model.step")
    fp_out = cli.abspath("out.step")
    code, stdout, stderr = cli.run("open-step", "--minify", "--save-to", fp_out, fp)
    if "LibrePCB was compiled without OpenCascade" in stderr:
        pytest.skip("Feature not available.")
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open STEP file '{fp}'...
Perform minify...
 - Minified from 28,521 bytes to 18,336 bytes (-36%)
Save to '{fp_out}'...
Load model...
SUCCESS
""")
    assert code == 0

    code, stdout, stderr = cli.run("open-step", "--minify", fp_out)
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open STEP file '{fp_out}'...
Perform minify...
 - File is already minified
Load model...
SUCCESS
""")
    assert code == 0
