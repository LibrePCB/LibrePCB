#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test command "open-library" (basic parser tests)
"""

HELP_TEXT = """\
Usage: {executable} [options] open-library [command_options] library
LibrePCB Command Line Interface

Options:
  -h, --help     Print this message.
  -V, --version  Displays version information.
  -v, --verbose  Verbose output.
  --all          Perform the selected action(s) on all elements contained in
                 the opened library.
  --save         Save library (and contained elements if '--all' is given)
                 before closing them (useful to upgrade file format).
  --strict       Fail if the opened files are not strictly canonical, i.e.
                 there would be changes when saving the library elements.

Arguments:
  open-library   Open a library to execute library-related tasks.
  library        Path to library directory (*.lplib).
"""

ERROR_TEXT = """\
{error}
Usage: {executable} [options] open-library [command_options] library
Help: {executable} open-library --help
"""


def test_help(cli):
    code, stdout, stderr = cli.run('open-library', '--help')
    assert stderr == ''
    assert stdout == HELP_TEXT.format(executable=cli.executable)
    assert code == 0


def test_no_arguments(cli):
    code, stdout, stderr = cli.run('open-library')
    assert stderr == ERROR_TEXT.format(
        executable=cli.executable,
        error="Missing arguments: library",
    )
    assert stdout == ''
    assert code == 1


def test_invalid_argument(cli):
    code, stdout, stderr = cli.run('open-library', '--invalid-argument')
    assert stderr == ERROR_TEXT.format(
        executable=cli.executable,
        error="Unknown option 'invalid-argument'.",
    )
    assert stdout == ''
    assert code == 1
