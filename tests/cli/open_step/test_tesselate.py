#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pytest
import re

"""
Test command "open-step --tesselate"
"""


PATTERN = "Open STEP file '{path}'...\\n" \
          "Load model...\\n" \
          "Tesselate model...\\n" \
          " - Built \\d\\d+ vertices with 1 different colors\\n" \
          "SUCCESS\\n"


def test_valid_file(cli):
    fp = cli.add_file('unittests/librepcbcommon/OccModelTest/model.step')
    code, stdout, stderr = cli.run('open-step', '--tesselate', fp)
    if 'LibrePCB was compiled without OpenCascade' in stderr:
        pytest.skip("Feature not available.")
    assert stderr == ''
    assert re.fullmatch(PATTERN.format(path=re.escape(fp)), stdout)
    assert code == 0
