#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import pytest

"""
Test command "open-project" with all available options
"""


@pytest.mark.parametrize("project_name,project_suffix,gerber_count", [
    ('Empty Project', 'lpp', 8),
    ('Empty Project', 'lppz', 8),
    ('Project With Two Boards', 'lpp', 16),
    ('Project With Two Boards', 'lppz', 16),
], ids=[
    'EmptyProject.lpp',
    'EmptyProject.lppz',
    'ProjectWithTwoBoards.lpp',
    'ProjectWithTwoBoards.lppz',
])
def test_everything(cli, project_name, project_suffix, gerber_count):
    # determine paths
    if project_suffix == 'lppz':
        project_path = 'data/' + project_name + '/' + project_name + '.lpp'
        output_dir = 'data/' + project_name + '/output/v1/gerber'
    else:
        project_path = 'data/' + project_name + '.lppz'
        output_dir = 'data/output/v1/gerber'

    # prepare "--export-schematics"
    schematic = cli.abspath('schematic.pdf')
    assert not os.path.exists(schematic)

    # prepare "--export-pcb-fabrication-data"
    output = cli.abspath(output_dir)
    assert not os.path.exists(output)

    # prepare "--save"
    path = cli.abspath(project_path)
    with open(path, 'ab') as f:
        f.write(b'\0\0')
    original_filesize = os.path.getsize(path)

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
    assert os.path.getsize(path) != original_filesize
