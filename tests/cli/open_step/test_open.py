#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pytest
from helpers import nofmt

"""
Test command "open-step"
"""


def test_valid_file(cli):
    fp = cli.add_file("unittests/librepcbcommon/OccModelTest/model.step")
    code, stdout, stderr = cli.run("open-step", fp)
    if "LibrePCB was compiled without OpenCascade" in stderr:
        pytest.skip("Feature not available.")
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open STEP file '{fp}'...
Load model...
SUCCESS
""")
    assert code == 0


def test_invalid_file(cli):
    fp = cli.add_file("LICENSE.txt")
    code, stdout, stderr = cli.run("open-step", fp)
    if "LibrePCB was compiled without OpenCascade" in stderr:
        pytest.skip("Feature not available.")
    assert len(stderr) > 10
    assert f"Open STEP file '{fp}'...\n" in stdout
    assert "Finished with errors!\n" in stdout
    assert code == 1
