#!/usr/bin/env python
# -*- coding: utf-8 -*-

import params
from helpers import nofmt

"""
Test command "open-project --strict"
"""


def test_valid_lpp(cli):
    project = params.EMPTY_PROJECT_LPP
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project', '--strict', project.path)
    assert stderr == ''
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Check for non-canonical files...
SUCCESS
""")
    assert code == 0


def test_invalid_lpp(cli):
    project = params.EMPTY_PROJECT_LPP
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    # append some zeros to the project file
    path = cli.abspath(project.path)
    with open(path, 'ab') as f:
        f.write(b'\0\0')
    # open project
    code, stdout, stderr = cli.run('open-project', '--strict', project.path)
    assert stderr == nofmt(f"""\
    - Non-canonical file: '{project.path}'
""")
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Check for non-canonical files...
Finished with errors!
""")
    assert code == 1


def test_lppz_fails(cli):
    project = params.PROJECT_WITH_TWO_BOARDS_LPPZ
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run('open-project', '--strict', project.path)
    assert stderr == nofmt("""\
  ERROR: The option '--strict' is not available for *.lppz files!
""")
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Check for non-canonical files...
Finished with errors!
""")
    assert code == 1
