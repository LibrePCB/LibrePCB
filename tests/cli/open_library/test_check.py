#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import params
import shutil

"""
Test command "open-library --check"
"""


def test_no_messages(cli):
    library = params.EMPTY_LIBRARY
    cli.add_library(library.dir)
    code, stdout, stderr = cli.run('open-library', '--all', '--check',
                                   library.dir)
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


def test_messages(cli):
    library = params.POPULATED_LIBRARY
    cli.add_library(library.dir)
    for subdir in ['sym', 'pkg', 'cmp']:
        shutil.rmtree(cli.abspath(os.path.join(library.dir, subdir)))
    code, stdout, stderr = cli.run('open-library', '--all', '--check',
                                   library.dir)
    assert stderr == \
        "  - PSMN022-30PL (5738d8f9-4101-4409-bd46-d9c173b40d60):\n" \
        "    - [ERROR] No categories set\n" \
        "  - PSMN5R8 (f7fb22e8-0bbc-4f0f-aa89-596823b5bc3e):\n" \
        "    - [ERROR] No categories set\n"
    assert stdout == \
        "Open library 'Populated Library.lplib'...\n" \
        "Process {library.cmpcat} component categories...\n" \
        "Process {library.pkgcat} package categories...\n" \
        "Process 0 symbols...\n" \
        "Process 0 packages...\n" \
        "Process 0 components...\n" \
        "Process {library.dev} devices...\n" \
        "Finished with errors!\n".format(library=library)
    assert code == 1
