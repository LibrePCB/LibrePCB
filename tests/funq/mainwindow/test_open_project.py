#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test opening projects from within main window
"""


def test_open_dialog(librepcb, helpers):
    """
    Open project by open dialog in main window
    """
    librepcb.add_project('Empty Project')
    with librepcb.open() as app:
        path = librepcb.abspath('Empty Project/Empty Project.lpp')
        app.widget('mainWindowTestAdapter').call_slot('trigger', 'project-open')
        app.widget('mainWindowOpenProjectDialogFileNameEdit').set_property('text', path)
        app.widget('mainWindowOpenProjectDialogOkButton').click()

        # Check if both editors were opened
        assert app.widget('schematicEditor').properties()['visible'] is True
        assert app.widget('boardEditor').properties()['visible'] is True

        # Check if the schematic editor is the active window
        helpers.wait_for_active_window(app, app.widget('schematicEditor'))  # raises on timeout
