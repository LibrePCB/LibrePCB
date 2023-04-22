#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
import pytest

"""
Test command "open-library --minify-step"
"""


def test_save(cli):
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    step = library.dir + \
        '/pkg/0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1' + \
        '/4e198b4d-b61a-47bd-a7ad-39b2f6dc77e9.step'
    path = cli.abspath(step)
    original_filesize = os.path.getsize(path)
    code, stdout, stderr = cli.run('open-library', '--all', '--minify-step',
                                   '--save', library.dir)
    if 'LibrePCB was compiled without OpenCascade' in stderr:
        pytest.skip("Feature not available.")
    filesize = os.path.getsize(path)
    assert stderr == ''
    assert stdout == \
        "Open library '{library.dir}'...\n" \
        "Process {library.cmpcat} component categories...\n" \
        "Process {library.pkgcat} package categories...\n" \
        "Process {library.sym} symbols...\n" \
        "Process {library.pkg} packages...\n" \
        "  - Minified '{step}' from {old} to {new} bytes\n" \
        "Process {library.cmp} components...\n" \
        "Process {library.dev} devices...\n" \
        "SUCCESS\n".format(library=library, step=step, old=original_filesize,
                           new=filesize).replace('/', os.sep)
    assert code == 0
    assert filesize < original_filesize


def test_strict(cli):
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    step = library.dir + \
        '/pkg/0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1' + \
        '/4e198b4d-b61a-47bd-a7ad-39b2f6dc77e9.step'
    path = cli.abspath(step)
    original_filesize = os.path.getsize(path)
    code, stdout, stderr = cli.run('open-library', '--all', '--minify-step',
                                   '--strict', library.dir)
    if 'LibrePCB was compiled without OpenCascade' in stderr:
        pytest.skip("Feature not available.")
    assert stderr == \
        "  - TO220AB (0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1):\n" \
        "    - Non-canonical file: '{step}'\n" \
        .format(step=step).replace('/', os.sep)
    assert "Minified '{}'".format(step).replace('/', os.sep) in stdout
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
    path = cli.abspath(step)
    with open(path, 'wb') as f:
        f.write(b'invalid')
    code, stdout, stderr = cli.run('open-library', '--all', '--minify-step',
                                   library.dir)
    if 'LibrePCB was compiled without OpenCascade' in stderr:
        pytest.skip("Feature not available.")
    assert stderr == \
        "  - TO220AB (0eaf289c-166d-4bd9-a4ba-dbf6bbc76ef1):\n" \
        "    - Failed to minify STEP model '{step}': STEP data section not found.\n" \
        .format(step=step).replace('/', os.sep)
    assert stdout == \
        "Open library '{library.dir}'...\n" \
        "Process {library.cmpcat} component categories...\n" \
        "Process {library.pkgcat} package categories...\n" \
        "Process {library.sym} symbols...\n" \
        "Process {library.pkg} packages...\n" \
        "Process {library.cmp} components...\n" \
        "Process {library.dev} devices...\n" \
        "Finished with errors!\n".format(library=library)
    assert code == 1
