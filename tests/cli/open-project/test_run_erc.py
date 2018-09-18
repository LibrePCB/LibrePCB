#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test command "open-project --erc"
"""

PROJECT_DIR = 'data/Empty Project/'
PROJECT_PATH = PROJECT_DIR + 'Empty Project.lpp'


def test_project_with_approved_message(cli):
    code, stdout, stderr = cli.run('open-project', '--erc', PROJECT_PATH)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert any(['Approved messages: 1' in line for line in stdout])
    assert any(['Non-approved messages: 0' in line for line in stdout])
    assert stdout[-1] == 'SUCCESS'


def test_project_without_messages(cli):
    # remove the unused netclass to suppress its ERC warning
    with open(cli.abspath(PROJECT_DIR + 'circuit/circuit.lp'), 'w') as f:
        f.write('(librepcb_circuit)')
    # also disapprove the ERC message to be sure it isn't ignored
    with open(cli.abspath(PROJECT_DIR + 'circuit/erc.lp'), 'w') as f:
        f.write('(librepcb_erc)')
    # now the actual test...
    code, stdout, stderr = cli.run('open-project', '--erc', PROJECT_PATH)
    assert code == 0
    assert len(stderr) == 0
    assert len(stdout) > 0
    assert any(['Approved messages: 0' in line for line in stdout])
    assert any(['Non-approved messages: 0' in line for line in stdout])
    assert stdout[-1] == 'SUCCESS'


def test_project_with_nonapproved_message(cli):
    # disapprove the ERC message
    with open(cli.abspath(PROJECT_DIR + 'circuit/erc.lp'), 'w') as f:
        f.write('(librepcb_erc)')
    # now the actual test...
    code, stdout, stderr = cli.run('open-project', '--erc', PROJECT_PATH)
    assert code == 1
    assert len(stderr) == 1
    assert 'Unused net class' in stderr[0]
    assert len(stdout) > 0
    assert any(['Approved messages: 0' in line for line in stdout])
    assert any(['Non-approved messages: 1' in line for line in stdout])
    assert stdout[-1] == 'Finished with errors!'
