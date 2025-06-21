#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
import pytest
from helpers import nofmt

"""
Test command "open-library --minify-step"
"""


def test_save(cli):
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    step = library.dir + \
        '/pkg/0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1' + \
        '/4e198b4d-b61a-47bd-a7ad-39b2f6dc77e9.step'
    step_native = step.replace('/', os.sep)
    path = cli.abspath(step)
    old_size = os.path.getsize(path)
    code, stdout, stderr = cli.run('open-library', '--all', '--minify-step',
                                   '--save', library.dir)
    if 'LibrePCB was compiled without OpenCascade' in stderr:
        pytest.skip("Feature not available.")
    new_size = os.path.getsize(path)
    assert stderr == ''
    assert stdout == nofmt(f"""\
Open library '{library.dir}'...
Process {library.cmpcat} component categories...
Process {library.pkgcat} package categories...
Process {library.sym} symbols...
Process {library.pkg} packages...
  - Minified '{step_native}' from {old_size} to {new_size} bytes
Process {library.cmp} components...
Process {library.dev} devices...
SUCCESS
""")
    assert code == 0
    assert new_size < old_size


def test_strict(cli):
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    step = library.dir + \
        '/pkg/0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1' + \
        '/4e198b4d-b61a-47bd-a7ad-39b2f6dc77e9.step'
    step_native = step.replace('/', os.sep)
    path = cli.abspath(step)
    original_filesize = os.path.getsize(path)
    code, stdout, stderr = cli.run('open-library', '--all', '--minify-step',
                                   '--strict', library.dir)
    if 'LibrePCB was compiled without OpenCascade' in stderr:
        pytest.skip("Feature not available.")
    assert stderr == nofmt(f"""\
  - TO220AB (0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1):
    - Non-canonical file: '{step_native}'
""")
    assert f"Minified '{step_native}'" in stdout
    assert "Finished with errors!" in stdout
    assert code == 1
    filesize = os.path.getsize(path)
    assert filesize == original_filesize


def test_invalid_file(cli):
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    step = library.dir + \
        '/pkg/0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1' + \
        '/4e198b4d-b61a-47bd-a7ad-39b2f6dc77e9.step'
    step_native = step.replace('/', os.sep)
    path = cli.abspath(step)
    with open(path, 'wb') as f:
        f.write(b'invalid')
    code, stdout, stderr = cli.run('open-library', '--all', '--minify-step',
                                   library.dir)
    if 'LibrePCB was compiled without OpenCascade' in stderr:
        pytest.skip("Feature not available.")
    assert stderr == nofmt(f"""\
  - TO220AB (0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1):
    - Failed to minify STEP model '{step_native}': STEP data section not found.
""")
    assert stdout == nofmt(f"""\
Open library '{library.dir}'...
Process {library.cmpcat} component categories...
Process {library.pkgcat} package categories...
Process {library.sym} symbols...
Process {library.pkg} packages...
Process {library.cmp} components...
Process {library.dev} devices...
Finished with errors!
""")
    assert code == 1
