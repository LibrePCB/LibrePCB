#!/usr/bin/env python
# -*- coding: utf-8 -*-

import platform
import pytest

"""
Test if the UI is translated according to the configured language
"""

LANGUAGE_ELEMENT_ID = "MainMenuBar::file-menu-item"
LANGUAGES = [
    ("en.utf8", "File"),
    ("de_DE.utf8", "Datei"),
    ("de_XX.utf8", "Datei"),  # Does not exist, should fall back to "de"
]

LOCALE_ELEMENT_ID = "StatusBar::grid-interval-edt"
LOCALES = [
    ("en_US", "0.0"),
    ("de_DE", "0,0"),
]


@pytest.mark.skipif(platform.system() != "Linux", reason="LC_ALL only works on Linux")
@pytest.mark.parametrize("language, expected", LANGUAGES)
def test_system_language(librepcb, language, expected):
    """
    Test if the system locale is respected for the translations.
    """
    librepcb.env["LC_ALL"] = language
    with librepcb.open() as app:
        assert app.get(LANGUAGE_ELEMENT_ID).label == expected


@pytest.mark.skipif(platform.system() != "Linux", reason="LC_ALL only works on Linux")
@pytest.mark.parametrize("locale, expected", LOCALES)
def test_system_locale(librepcb, locale, expected):
    """
    Test if the system locale is respected for number formatting.
    """
    librepcb.env["LC_ALL"] = locale
    with librepcb.open() as app:
        assert app.get(LOCALE_ELEMENT_ID).value == expected


@pytest.mark.parametrize("language, expected_translation",
    # strip the ".utf8" from the locale code
    [(language.split(".")[0], expected) for language, expected in LANGUAGES]
)
@pytest.mark.parametrize("locale, expected_locale", LOCALES)
def test_workspace_language(
    librepcb, language, expected_translation, locale, expected_locale
):
    """
    Check if the language set in the workspace settings change the UI language,
    but not the locale. See https://github.com/LibrePCB/LibrePCB/pull/1723.
    """
    librepcb.env["LC_ALL"] = locale
    with open(librepcb.get_workspace_path("data/settings.lp"), mode="w") as f:
        f.write(f'(librepcb_workspace_settings (application_locale "{language}"))')
    with librepcb.open() as app:
        assert app.get(LANGUAGE_ELEMENT_ID).label == expected_translation
        if platform.system() == "Linux":
            assert app.get(LOCALE_ELEMENT_ID).value == expected_locale
