#!/usr/bin/env python
# -*- coding: utf-8 -*-

import params
import pytest

"""
Test command "open-project --set-default-variant"
"""


@pytest.mark.parametrize("project", [
    params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM,
    params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM,
])
def test_project_with_two_boards_save(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project',
                                   '--set-default-variant=AV',
                                   '--save',
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Set default assembly variant to 'AV'...\n" \
        "Save project...\n" \
        "SUCCESS\n".format(project=project)
    assert code == 0

    # Open again to check that the assembly variant still exists.
    code, stdout, stderr = cli.run('open-project',
                                   '--set-default-variant=AV',
                                   project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Set default assembly variant to 'AV'...\n" \
        "SUCCESS\n".format(project=project)
    assert code == 0


@pytest.mark.parametrize("project", [params.PROJECT_WITH_TWO_BOARDS_LPP_PARAM])
def test_invalid_variant(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project',
                                   '--set-default-variant=Foo',
                                   '--save',
                                   project.path)
    assert stderr == \
        "ERROR: No assembly variant with the name 'Foo' found.\n"
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Set default assembly variant to 'Foo'...\n" \
        "Save project...\n" \
        "Finished with errors!\n".format(project=project)
    assert code == 1
