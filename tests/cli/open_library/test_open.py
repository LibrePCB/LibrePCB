#!/usr/bin/env python
# -*- coding: utf-8 -*-

import params
import pytest
from helpers import nofmt

"""
Test command "open-library"
"""


@pytest.mark.parametrize(
    "library",
    [
        params.EMPTY_LIBRARY_PARAM,
        params.POPULATED_LIBRARY_PARAM,
    ],
)
def test_open_library_absolute_path(cli, library):
    cli.add_library(library.dir)
    code, stdout, stderr = cli.run("open-library", cli.abspath(library.dir))
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open library '{cli.abspath(library.dir)}'...
SUCCESS
""")
    assert code == 0


@pytest.mark.parametrize(
    "library",
    [
        params.EMPTY_LIBRARY_PARAM,
        params.POPULATED_LIBRARY_PARAM,
    ],
)
def test_open_library_relative_path(cli, library):
    cli.add_library(library.dir)
    code, stdout, stderr = cli.run("open-library", library.dir)
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open library '{library.dir}'...
SUCCESS
""")
    assert code == 0


@pytest.mark.parametrize(
    "library",
    [
        params.EMPTY_LIBRARY_PARAM,
        params.POPULATED_LIBRARY_PARAM,
    ],
)
def test_open_library_all(cli, library):
    cli.add_library(library.dir)
    code, stdout, stderr = cli.run("open-library", "--all", library.dir)
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open library '{library.dir}'...
Process {library.cmpcat} component categories...
Process {library.pkgcat} package categories...
Process {library.sym} symbols...
Process {library.pkg} packages...
Process {library.cmp} components...
Process {library.dev} devices...
SUCCESS
""")
    assert code == 0


@pytest.mark.parametrize(
    "library",
    [
        params.POPULATED_LIBRARY_PARAM,
    ],
)
def test_open_library_all_verbose(cli, library):
    cli.add_library(library.dir)
    code, stdout, stderr = cli.run("open-library", "--all", "--verbose", library.dir)
    assert len(stderr) > 100  # logging messages are on stderr
    assert stdout == nofmt(f"""\
Open library '{library.dir}'...
Process {library.cmpcat} component categories...
Process {library.pkgcat} package categories...
Process {library.sym} symbols...
Process {library.pkg} packages...
Process {library.cmp} components...
Process {library.dev} devices...
SUCCESS
""")
    assert code == 0
