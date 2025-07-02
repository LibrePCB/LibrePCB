#!/usr/bin/env python
# -*- coding: utf-8 -*-
import re


def nofmt(arg):
    """
    Just a no-op function to avoid Ruff from formatting assert statements
    in a very ugly way.
    """
    return arg


def strip_image_file_extensions(help_text):
    """
    Remove client-dependent image file extensions from the help text to make
    the tests portable.
    """
    return re.sub(
        "extensions: pdf, svg,([\\s\\n]*[0-9a-z,]+)+",
        "extensions: pdf, svg, ***",
        help_text,
    )
