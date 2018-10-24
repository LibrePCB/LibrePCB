#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test opening the library manager
"""


def test_using_button(librepcb):
    """
    Open library manager with the button in the control panel
    """
    with librepcb.open() as app:
        app.widget('controlPanelOpenLibraryManagerButton').click()
        assert app.widget('libraryManager').properties()['visible'] is True


def test_using_menu(librepcb):
    """
    Open library manager with the menu item in the control panel
    """
    with librepcb.open() as app:
        app.action('controlPanelActionOpenLibraryManager').trigger()
        assert app.widget('libraryManager').properties()['visible'] is True


# TODO: How to emulate a click on the hyperlink in a QLabel?
# def test_using_no_libraries_warning(librepcb):
#     """
#     Open library manager with the link in the "no libraries installed" warning
#     """
#     with librepcb.open() as app:
#         label = app.widget('controlPanelWarnForNoLibrariesLabel')
#         assert app.widget('libraryManager').properties()['visible'] is True
