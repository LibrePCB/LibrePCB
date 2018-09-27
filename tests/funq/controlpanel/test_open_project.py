#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test opening projects
"""

project = 'data/fixtures/Empty Project/Empty Project.lpp'


def test_command_line_argument(librepcb):
    """
    Open project by command line argument
    """
    librepcb.set_project(project)
    with librepcb.open() as app:
        assert app.widget('schematicEditor').properties()['visible'] is True
        assert app.widget('boardEditor').properties()['visible'] is True


def test_open_dialog(librepcb):
    """
    Open project by open dialog in control panel
    """
    with librepcb.open() as app:
        path = librepcb.abspath(project)
        app.widget('controlPanelOpenProjectButton').click()
        app.widget('controlPanelOpenProjectDialogFileNameEdit').set_property('text', path)
        app.widget('controlPanelOpenProjectDialogOkButton').click()
        assert app.widget('schematicEditor').properties()['visible'] is True
        assert app.widget('boardEditor').properties()['visible'] is True
