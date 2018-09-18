#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

"""
Test command "open-project --export-schematics"
"""

PROJECT_DIR = 'data/Empty Project/'
PROJECT_PATH = PROJECT_DIR + 'Empty Project.lpp'


def test_if_unknown_file_extension_fails(cli):
    code, stdout, stderr = cli.run('open-project',
                                   '--export-schematics=foo.bar',
                                   PROJECT_PATH)
    assert code == 1
    assert len(stderr) == 1
    assert 'Unknown extension' in stderr[0]
    assert len(stdout) > 0
    assert stdout[-1] == 'Finished with errors!'


def test_exporting_pdf_with_relative_path(cli):
    path = cli.abspath('sch.pdf')
    assert not os.path.exists(path)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-schematics=sch.pdf',
                                   PROJECT_PATH)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(path)


def test_exporting_pdf_with_absolute_path(cli):
    path = cli.abspath('schematic with spaces.pdf')
    assert not os.path.exists(path)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-schematics={}'.format(path),
                                   PROJECT_PATH)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(path)


def test_if_output_directories_are_created(cli):
    dir = cli.abspath('nonexistent directory/nested')
    path = os.path.join(dir, 'schematic.pdf')
    assert not os.path.exists(dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--export-schematics={}'.format(path),
                                   PROJECT_PATH)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'
    assert os.path.exists(dir)
    assert os.path.exists(path)
