#!/usr/bin/env python
# -*- coding: utf-8 -*-

import re

"""
Test "--version"
"""

PLAIN_PATTERN = "LibrePCB CLI Version \\d+\\.\\d+\\.\\d+(\\-\\w+)?\\n" \
                "File Format [0-9]+(\\.[0-9]+)? \\((stable|unstable)\\)\\n" \
                "Git Revision [0-9a-f]+\\n" \
                "Qt Version [0-9\\.]+ \\(compiled against [0-9\\.]+\\)\\n" \
                "Built at [0-9a-zA-Z\\.:/ ]+\\n"

JSON_PATTERN = """\
\\{
    "app": \\{
        "revision": "[0-9a-f]+",
        "version": "\\d+\\.\\d+\\.\\d+(\\-\\w+)?"
    \\},
    "build": \\{
        "date": "\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}"
    \\},
    "file_format": \\{
        "stable": (true|false),
        "version": "[0-9]+(\\.[0-9]+)?"
    \\},
    "qt": \\{
        "compiled": "[0-9\\.]+",
        "linked": "[0-9\\.]+"
    \\}
\\}
""".replace('\n', '\\n')


def test_plain(cli):
    code, stdout, stderr = cli.run('--version')
    assert stderr == ''
    assert re.fullmatch(PLAIN_PATTERN, stdout)
    assert code == 0


def test_json(cli):
    code, stdout, stderr = cli.run('--version', '--json')
    assert stderr == ''
    assert re.fullmatch(JSON_PATTERN, stdout)
    assert code == 0
