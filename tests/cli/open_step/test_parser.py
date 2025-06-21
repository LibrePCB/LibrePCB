#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test command "open-step" (basic parser tests)
"""

HELP_TEXT = """\
Usage: {executable} [options] open-step [command_options] file
LibrePCB Command Line Interface

Options:
  -h, --help        Print this message.
  -V, --version     Displays version information.
  -v, --verbose     Verbose output.
  --minify          Minify the STEP model before validating it. Use in
                    conjunction with '--save-to' to save the output of the
                    operation.
  --tesselate       Tesselate the loaded STEP model to check if LibrePCB is
                    able to render it. Reports failure (exit code = 1) if no
                    content is detected.
  --save-to <file>  Write the (modified) STEP file to this output location (may
                    be equal to the opened file path). Only makes sense in
                    conjunction with '--minify'.

Arguments:
  open-step         Open a STEP model to execute STEP-related tasks outside of
                    a library.
  file              Path to the STEP file (*.step).
"""

ERROR_TEXT = """\
{error}
Usage: {executable} [options] open-step [command_options] file
Help: {executable} open-step --help
"""


def test_help(cli):
    code, stdout, stderr = cli.run("open-step", "--help")
    assert stderr == ""
    assert stdout == HELP_TEXT.format(executable=cli.executable)
    assert code == 0


def test_no_arguments(cli):
    code, stdout, stderr = cli.run("open-step")
    assert stderr == ERROR_TEXT.format(
        executable=cli.executable,
        error="Missing arguments: file",
    )
    assert stdout == ""
    assert code == 1


def test_invalid_argument(cli):
    code, stdout, stderr = cli.run("open-step", "--invalid-argument")
    assert stderr == ERROR_TEXT.format(
        executable=cli.executable,
        error="Unknown option 'invalid-argument'.",
    )
    assert stdout == ""
    assert code == 1
