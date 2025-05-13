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
        app.widget('openProjectDialogFileNameEdit').set_property('text', path)
        app.widget('openProjectDialogOkButton').click()
        helpers.wait_for_project(app, 'Empty Project')
