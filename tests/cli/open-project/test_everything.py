#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import pytest

"""
Test command "open-project" with all available options
"""

PROJECT_DIR = 'data/Empty Project/'
PROJECT_PATH = PROJECT_DIR + 'Empty Project.lpp'
OUTPUT_DIR = PROJECT_DIR + 'output/v1/gerber'

PROJECT_2_DIR = 'data/Project With Two Boards/'
PROJECT_2_PATH = PROJECT_2_DIR + 'Project With Two Boards.lpp'
OUTPUT_2_DIR = PROJECT_2_DIR + 'output/v1/gerber'


@pytest.mark.parametrize("project_path,output_dir,gerber_count", [
    (PROJECT_PATH, OUTPUT_DIR, 8),
    (PROJECT_2_PATH, OUTPUT_2_DIR, 16),
], ids=[
    'EmptyProject',
    'ProjectWithTwoBoards'
])
def test_everything(cli, project_path, output_dir, gerber_count):
    # prepare "--export-schematics"
    schematic = cli.abspath('schematic.pdf')
    assert not os.path.exists(schematic)

    # prepare "--export-pcb-fabrication-data"
    output = cli.abspath(output_dir)
    assert not os.path.exists(output)

    # prepare "--save"
    path = cli.abspath(project_path)
    original_filesize = os.path.getsize(path)
    with open(path, 'ab') as f:
        f.write(b'\n\n')
    assert os.path.getsize(path) == original_filesize + 2

    # run the CLI
    code, stdout, stderr = cli.run('open-project',
                                   '--erc',
                                   '--export-schematics={}'.format(schematic),
                                   '--export-pcb-fabrication-data',
                                   '--save',
                                   project_path)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert stdout[-1] == 'SUCCESS'

    # check "--export-schematics"
    assert os.path.exists(schematic)

    # check "--export-pcb-fabrication-data"
    assert os.path.exists(output)
    assert len(os.listdir(output)) == gerber_count

    # check "--save"
    assert os.path.getsize(path) == original_filesize
