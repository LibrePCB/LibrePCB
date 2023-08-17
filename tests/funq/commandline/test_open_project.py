#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test opening projects by command line arguments
"""


def test_open_lpp(librepcb, helpers):
    """
    Open *.lpp project by command line argument
    """
    librepcb.add_project('Empty Project')
    librepcb.set_project('Empty Project/Empty Project.lpp')
    with librepcb.open() as app:
        # Check if both editors were opened
        assert app.widget('schematicEditor').properties()['visible'] is True
        assert app.widget('boardEditor').properties()['visible'] is True

        # Check if the schematic editor is the active window
        helpers.wait_for_active_window(app, app.widget('schematicEditor'))  # raises on timeout


def test_open_lppz(librepcb, helpers):
    """
    Open *.lppz project by command line argument
    """
    librepcb.add_project('Empty Project', as_lppz=True)
    librepcb.set_project('Empty Project.lppz')
    with librepcb.open() as app:
        # Check if both editors were opened
        assert app.widget('schematicEditor').properties()['visible'] is True
        assert app.widget('boardEditor').properties()['visible'] is True

        # Check if the schematic editor is the active window
        helpers.wait_for_active_window(app, app.widget('schematicEditor'))  # raises on timeout
