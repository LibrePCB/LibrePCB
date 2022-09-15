#!/usr/bin/env python
# -*- coding: utf-8 -*-

import params
import pytest

"""
Test command "open-project --erc"
"""


@pytest.mark.parametrize("project", [params.EMPTY_PROJECT_LPP_PARAM])
def test_project_with_approved_message(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project', '--erc', project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Run ERC...\n" \
        "  Approved messages: 1\n" \
        "  Non-approved messages: 0\n" \
        "SUCCESS\n".format(project=project)
    assert code == 0


@pytest.mark.parametrize("project", [params.EMPTY_PROJECT_LPP_PARAM])
def test_project_without_messages(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    # remove the unused netclass to suppress its ERC warning
    with open(cli.abspath(project.dir + '/circuit/circuit.lp'), 'w') as f:
        f.write('(librepcb_circuit)')
    # also disapprove the ERC message to be sure it isn't ignored
    with open(cli.abspath(project.dir + '/circuit/erc.lp'), 'w') as f:
        f.write('(librepcb_erc)')
    # now the actual test...
    code, stdout, stderr = cli.run('open-project', '--erc', project.path)
    assert stderr == ''
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Run ERC...\n" \
        "  Approved messages: 0\n" \
        "  Non-approved messages: 0\n" \
        "SUCCESS\n".format(project=project)
    assert code == 0


@pytest.mark.parametrize("project", [params.EMPTY_PROJECT_LPP_PARAM])
def test_project_with_nonapproved_message(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    # disapprove the ERC message
    with open(cli.abspath(project.dir + '/circuit/erc.lp'), 'w') as f:
        f.write('(librepcb_erc)')
    # now the actual test...
    code, stdout, stderr = cli.run('open-project', '--erc', project.path)
    assert stderr == '    - [WARNING] Unused net class: "default"\n'
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Run ERC...\n" \
        "  Approved messages: 0\n" \
        "  Non-approved messages: 1\n" \
        "Finished with errors!\n".format(project=project)
    assert code == 1
