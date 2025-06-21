#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test "--help"

Note: It might look a bit picky to compare the CLI output with *exact* values,
but in the end it's very easy to update the expected values after each change
and it heavily increases test coverage.
"""

HELP_TEXT = """\
Usage: {executable} [options] command
LibrePCB Command Line Interface

Options:
  -h, --help     Print this message.
  -V, --version  Displays version information.
  -v, --verbose  Verbose output.

Arguments:
  command        The command to execute (see list below).

Commands:
  open-library   Open a library to execute library-related tasks.
  open-project   Open a project to execute project-related tasks.
  open-step      Open a STEP model to execute STEP-related tasks outside of a library.

List command-specific options:
  {executable} <command> --help
"""

ERROR_TEXT = """\
{error}
Usage: {executable} [options] command
Help: {executable} --help
"""


def test_explicit(cli):
    code, stdout, stderr = cli.run("--help")
    assert stderr == ""
    assert stdout == HELP_TEXT.format(executable=cli.executable)
    assert code == 0


def test_implicit_if_no_arguments(cli):
    code, stdout, stderr = cli.run()
    assert stderr == ""
    assert stdout == HELP_TEXT.format(executable=cli.executable)
    assert code == 0


def test_implicit_if_passing_invalid_command(cli):
    code, stdout, stderr = cli.run("invalid-command")
    assert stderr == ERROR_TEXT.format(
        executable=cli.executable,
        error="Unknown command 'invalid-command'.",
    )
    assert stdout == ""
    assert code == 1


def test_implicit_if_passing_invalid_argument(cli):
    code, stdout, stderr = cli.run("--invalid-argument")
    assert stderr == ERROR_TEXT.format(
        executable=cli.executable,
        error="Unknown option 'invalid-argument'.",
    )
    assert stdout == ""
    assert code == 1
