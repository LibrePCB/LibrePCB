#!/usr/bin/env python
# -*- coding: utf-8 -*-

import re

"""
Test command "open-project" (basic parser tests)
"""

HELP_TEXT = """\
Usage: {executable} [options] open-project [command_options] project
LibrePCB Command Line Interface

Options:
  -h, --help                         Print this message.
  -V, --version                      Displays version information.
  -v, --verbose                      Verbose output.
  --erc                              Run the electrical rule check, print all
                                     non-approved warnings/errors and report
                                     failure (exit code = 1) if there are
                                     non-approved messages.
  --drc                              Run the design rule check, print all
                                     non-approved warnings/errors and report
                                     failure (exit code = 1) if there are
                                     non-approved messages.
  --drc-settings <file>              Override DRC settings by providing a *.lp
                                     file containing custom settings. If not
                                     set, the settings from the boards will be
                                     used instead.
  --export-schematics <file>         Export schematics to given file(s).
                                     Existing files will be overwritten.
                                     Supported file extensions: pdf, svg, ***
  --export-bom <file>                Export generic BOM to given file(s).
                                     Existing files will be overwritten.
                                     Supported file extensions: csv
  --export-board-bom <file>          Export board-specific BOM to given
                                     file(s). Existing files will be
                                     overwritten. Supported file extensions: csv
  --bom-attributes <attributes>      Comma-separated list of additional
                                     attributes to be exported to the BOM.
                                     Example: "SUPPLIER, SKU"
  --export-pcb-fabrication-data      Export PCB fabrication data
                                     (Gerber/Excellon) according the fabrication
                                     output settings of boards. Existing files
                                     will be overwritten.
  --pcb-fabrication-settings <file>  Override PCB fabrication output settings
                                     by providing a *.lp file containing custom
                                     settings. If not set, the settings from the
                                     boards will be used instead.
  --export-pnp-top <file>            Export pick&place file for automated
                                     assembly of the top board side. Existing
                                     files will be overwritten. Supported file
                                     extensions: csv, gbr
  --export-pnp-bottom <file>         Export pick&place file for automated
                                     assembly of the bottom board side. Existing
                                     files will be overwritten. Supported file
                                     extensions: csv, gbr
  --export-netlist <file>            Export netlist file for automated PCB
                                     testing. Existing files will be
                                     overwritten. Supported file extensions:
                                     d356
  --board <name>                     The name of the board(s) to export. Can be
                                     given multiple times. If not set, all
                                     boards are exported.
  --board-index <index>              Same as '--board', but allows to specify
                                     boards by index instead of by name.
  --remove-other-boards              Remove all boards not specified with
                                     '--board[-index]' from the project before
                                     executing all the other actions. If
                                     '--board[-index]' is not passed, all boards
                                     will be removed. Pass '--save' to save the
                                     modified project to disk.
  --variant <name>                   The name of the assembly variant(s) to
                                     export. Can be given multiple times. If not
                                     set, all assembly variants are exported.
  --variant-index <index>            Same as '--variant', but allows to specify
                                     assembly variants by index instead of by
                                     name.
  --set-default-variant <name>       Move the specified assembly variant to the
                                     top before executing all the other actions.
                                     Pass '--save' to save the modified project
                                     to disk.
  --save                             Save project before closing it (useful to
                                     upgrade file format).
  --strict                           Fail if the project files are not strictly
                                     canonical, i.e. there would be changes when
                                     saving the project. Note that this option
                                     is not available for *.lppz files.

Arguments:
  open-project                       Open a project to execute project-related
                                     tasks.
  project                            Path to project file (*.lpp[z]).
"""

ERROR_TEXT = """\
{error}
Usage: {executable} [options] open-project [command_options] project
Help: {executable} open-project --help
"""


def _clean(help_text):
    """
    Remove client-dependent image file extensions from the help text to make
    the tests portable.
    """
    return re.sub(
        'Supported file extensions: pdf, svg,([\\s\\n]*[0-9a-z,]+)+',
        'Supported file extensions: pdf, svg, ***',
        help_text,
    )


def test_help(cli):
    code, stdout, stderr = cli.run('open-project', '--help')
    assert stderr == ''
    assert _clean(stdout) == HELP_TEXT.format(executable=cli.executable)
    assert code == 0


def test_no_arguments(cli):
    code, stdout, stderr = cli.run('open-project')
    assert stderr == ERROR_TEXT.format(
        executable=cli.executable,
        error="Missing arguments: project",
    )
    assert stdout == ''
    assert code == 1


def test_invalid_argument(cli):
    code, stdout, stderr = cli.run('open-project', '--invalid-argument')
    assert stderr == ERROR_TEXT.format(
        executable=cli.executable,
        error="Unknown option 'invalid-argument'.",
    )
    assert stdout == ''
    assert code == 1
