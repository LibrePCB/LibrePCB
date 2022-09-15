#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
import pytest

"""
Test command "open-project --remove-other-boards"
"""


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_project_with_two_boards_remove_both_volatile(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    # Without passing '--save', the removal is not persistent.
    for _ in range(2):
        code, stdout, stderr = cli.run('open-project',
                                       '--remove-other-boards',
                                       project.path)
        assert stderr == ''
        assert stdout == \
            "Open project '{project.path}'...\n" \
            "Remove other boards...\n" \
            "  - 'default'\n" \
            "  - 'copy'\n" \
            "SUCCESS\n".format(project=project)
        assert code == 0


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_project_with_two_boards_remove_one_and_save(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    removed_dir = cli.abspath(project.dir + '/boards/default')
    if not project.is_lppz:
        assert os.path.exists(removed_dir)
    code, stdout, stderr = cli.run('open-project',
                                   '--board=copy',
                                   '--remove-other-boards',
                                   '--save',
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Remove other boards...\n" \
        "  - 'default'\n" \
        "Save project...\n" \
        "SUCCESS\n".format(project=project)
    assert code == 0
    if not project.is_lppz:
        assert os.path.exists(removed_dir) is False

    # Open again to check that the removed board is not loaded anymore.
    code, stdout, stderr = cli.run('open-project',
                                   '--board=copy',
                                   '--remove-other-boards',
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Remove other boards...\n" \
        "SUCCESS\n".format(project=project)
    assert code == 0
