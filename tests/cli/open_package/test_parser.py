#!/usr/bin/env python
# -*- coding: utf-8 -*-

import re

"""
Test command "open-package" (basic parser tests)
"""

HELP_TEXT = """\
Usage: {executable} [options] open-package [command_options] package
LibrePCB Command Line Interface

Options:
  -h, --help       Print this message.
  -V, --version    Displays version information.
  -v, --verbose    Verbose output.
  --check          Run the package check, print all non-approved messages and
                   report failure (exit code = 1) if there are non-approved
                   messages.
  --export <file>  Export the contained footprint(s) to a graphical file.
                   Supported file extensions: pdf, svg, ***

Arguments:
  open-package     Open a package to execute package-related tasks.
  package          Path to package directory (containing *.lp).
"""

ERROR_TEXT = """\
{error}
Usage: {executable} [options] open-package [command_options] package
Help: {executable} open-package --help
"""


def _clean(help_text):
    """
    Remove client-dependent image file extensions from the help text to make
    the tests portable.
    """
    return re.sub(
        "extensions: pdf, svg,([\\s\\n]*[0-9a-z,]+)+",
        "extensions: pdf, svg, ***",
        help_text,
    )


def test_help(cli):
    code, stdout, stderr = cli.run("open-package", "--help")
    assert stderr == ""
    assert _clean(stdout) == HELP_TEXT.format(executable=cli.executable)
    assert code == 0


def test_no_arguments(cli):
    code, stdout, stderr = cli.run("open-package")
    assert stderr == ERROR_TEXT.format(
        executable=cli.executable,
        error="Missing arguments: package",
    )
    assert stdout == ""
    assert code == 1


def test_invalid_argument(cli):
    code, stdout, stderr = cli.run("open-package", "--invalid-argument")
    assert stderr == ERROR_TEXT.format(
        executable=cli.executable,
        error="Unknown option 'invalid-argument'.",
    )
    assert stdout == ""
    assert code == 1
