#!/usr/bin/env python
# -*- coding: utf-8 -*-

import params
import pytest

"""
Test command "open-library"
"""


@pytest.mark.parametrize("library", [
    params.EMPTY_LIBRARY_PARAM,
    params.POPULATED_LIBRARY_PARAM,
])
def test_open_library_absolute_path(cli, library):
    cli.add_library(library.dir)
    code, stdout, stderr = cli.run('open-library', cli.abspath(library.dir))
    assert stderr == ''
    assert stdout == \
        "Open library '{dir}'...\n" \
        "SUCCESS\n".format(dir=cli.abspath(library.dir))
    assert code == 0


@pytest.mark.parametrize("library", [
    params.EMPTY_LIBRARY_PARAM,
    params.POPULATED_LIBRARY_PARAM,
])
def test_open_library_relative_path(cli, library):
    cli.add_library(library.dir)
    code, stdout, stderr = cli.run('open-library', library.dir)
    assert stderr == ''
    assert stdout == \
        "Open library '{library.dir}'...\n" \
        "SUCCESS\n".format(library=library)
    assert code == 0


@pytest.mark.parametrize("library", [
    params.EMPTY_LIBRARY_PARAM,
    params.POPULATED_LIBRARY_PARAM,
])
def test_open_library_all(cli, library):
    cli.add_library(library.dir)
    code, stdout, stderr = cli.run('open-library', '--all', library.dir)
    assert stderr == ''
    assert stdout == \
        "Open library '{library.dir}'...\n" \
        "Process {library.cmpcat} component categories...\n" \
        "Process {library.pkgcat} package categories...\n" \
        "Process {library.sym} symbols...\n" \
        "Process {library.pkg} packages...\n" \
        "Process {library.cmp} components...\n" \
        "Process {library.dev} devices...\n" \
        "SUCCESS\n".format(library=library)
    assert code == 0


@pytest.mark.parametrize("library", [
    params.POPULATED_LIBRARY_PARAM,
])
def test_open_library_all_verbose(cli, library):
    cli.add_library(library.dir)
    code, stdout, stderr = cli.run('open-library', '--all', '--verbose',
                                   library.dir)
    assert len(stderr) > 100  # logging messages are on stderr
    assert stdout == \
        "Open library '{library.dir}'...\n" \
        "Process {library.cmpcat} component categories...\n" \
        "Process {library.pkgcat} package categories...\n" \
        "Process {library.sym} symbols...\n" \
        "Process {library.pkg} packages...\n" \
        "Process {library.cmp} components...\n" \
        "Process {library.dev} devices...\n" \
        "SUCCESS\n".format(library=library)
    assert code == 0
