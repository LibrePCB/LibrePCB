#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test opening the library manager
"""


def test(librepcb):
    """
    Open library manager with the shortcut
    """
    with librepcb.open() as app:
        app.action('mainWindowActionOpenLibraryManager').trigger()
        assert app.widget('libraryManager').properties()['visible'] is True
