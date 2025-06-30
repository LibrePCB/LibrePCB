#!/usr/bin/env python
# -*- coding: utf-8 -*-

import re

"""
Test "--version"
"""

PATTERN = (
    "LibrePCB CLI Version \\d+\\.\\d+\\.\\d+(\\-\\w+)?\\n"
    "File Format [0-9]+(\\.[0-9]+)? \\((stable|unstable)\\)\\n"
    "Git Revision [0-9a-f]+ \\([0-9-]+\\)\\n"
    "Qt Version [0-9\\.]+ \\(compiled against [0-9\\.]+\\)\\n"
    "OpenCascade [A-Z/]+( [0-9\\.]+)?\\n"
)


def test_valid_call(cli):
    code, stdout, stderr = cli.run("--version")
    assert stderr == ""
    assert re.fullmatch(PATTERN, stdout)
    assert code == 0
