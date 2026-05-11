#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test keyboard shortcuts
"""

import platform
import pytest
from slint_testing import keys


@pytest.mark.skipif(
    platform.system() == "Windows",
    reason="Ctrl+Alt shortcuts currently don't work on Windows",
)
def test_default_shortcuts(librepcb):
    with librepcb.open() as app:
        app.trigger_shortcut([keys.Control, keys.Alt, "m"])
        app.get("#LibrariesPanel")


def test_custom_shortcuts(librepcb):
    librepcb.set_keyboard_shortcut("library_manager", "F6")
    with librepcb.open() as app:
        app.trigger_shortcut([keys.F6])
        app.get("#LibrariesPanel")
