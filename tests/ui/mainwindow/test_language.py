#!/usr/bin/env python
# -*- coding: utf-8 -*-

import platform
import pytest

"""
Test if the UI is translated according to the configured language
"""

ELEMENT_ID = "MainMenuBar::file-menu-item"
LANGUAGES = [
    ("C", "File"),
    ("en.utf8", "File"),
    ("en_US.utf8", "File"),
    ("de.utf8", "Datei"),
    ("de_CH.utf8", "Datei"),
]


@pytest.mark.skipif(platform.system() != "Linux", reason="LC_ALL only works on Linux")
@pytest.mark.parametrize("locale, translation", LANGUAGES)
def test_system_language(librepcb, locale, translation):
    librepcb.env["LC_ALL"] = locale
    with librepcb.open() as app:
        assert app.get(ELEMENT_ID).label == translation
