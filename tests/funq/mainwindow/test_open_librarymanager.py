#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test opening the library manager
"""


def test(librepcb):
    """
    Open library manager
    """
    with librepcb.open() as app:
        app.widget('mainWindowTestAdapter').call_slot('trigger', 'library-manager')
        assert app.widget('libraryManager').properties()['visible'] is True
