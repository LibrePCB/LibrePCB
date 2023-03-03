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
    # Add a unused net.
    with open(cli.abspath(project.dir + '/circuit/circuit.lp'), 'w') as f:
        f.write(
            '(librepcb_circuit'
            ' (netclass ada6400e-cb5c-41b6-9a4e-e2f39ffa3383 (name "C"))'
            ' (net 0654411b-a090-4025-a026-4c61e686662e (auto false) (name "N")'
            '  (netclass ada6400e-cb5c-41b6-9a4e-e2f39ffa3383)'
            ' )'
            ')'
        )
    # Approve the resulting message.
    with open(cli.abspath(project.dir + '/circuit/erc.lp'), 'w') as f:
        f.write(
            '(librepcb_erc'
            ' (approved open_net (net 0654411b-a090-4025-a026-4c61e686662e))'
            ')'
        )
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
    # Make sure there are no approved messages.
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
    # Add a unused net.
    with open(cli.abspath(project.dir + '/circuit/circuit.lp'), 'w') as f:
        f.write(
            '(librepcb_circuit'
            ' (netclass ada6400e-cb5c-41b6-9a4e-e2f39ffa3383 (name "C"))'
            ' (net 0654411b-a090-4025-a026-4c61e686662e (auto false) (name "N")'
            '  (netclass ada6400e-cb5c-41b6-9a4e-e2f39ffa3383)'
            ' )'
            ')'
        )
    # Make sure there are no approved messages.
    with open(cli.abspath(project.dir + '/circuit/erc.lp'), 'w') as f:
        f.write('(librepcb_erc)')
    # now the actual test...
    code, stdout, stderr = cli.run('open-project', '--erc', project.path)
    assert stderr == "    - [WARNING] Less than two pins in net: 'N'\n"
    assert stdout == \
        "Open project '{project.path}'...\n" \
        "Run ERC...\n" \
        "  Approved messages: 0\n" \
        "  Non-approved messages: 1\n" \
        "Finished with errors!\n".format(project=project)
    assert code == 1
