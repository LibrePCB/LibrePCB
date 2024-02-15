#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test creating projects
"""

import os


def test_new_project_wizard(librepcb):
    """
    Create project using the wizard from the control panel
    """
    with librepcb.open() as app:
        # Open new project wizard
        app.widget('controlPanelNewProjectButton').click()
        assert app.widget('controlPanelNewProjectWizard').properties()['visible'] is True
        # Enter metadata
        name = 'New Project'
        app.widget('controlPanelNewProjectWizardMetadataNameEdit').set_property('text', name)
        path = app.widget('controlPanelNewProjectWizardMetadataPathEdit').properties()['text']
        app.widget('controlPanelNewProjectWizardNextButton').click()
        # Setup schematic/board
        app.widget('controlPanelNewProjectWizardFinishButton').click()
        # Verify if editors are opened and project file exists
        assert app.widget('schematicEditor').properties()['visible'] is True
        assert app.widget('boardEditor').properties()['visible'] is True
        assert os.path.exists(path)

    # Open project again to see if it was saved properly
    librepcb.set_project(path)
    with librepcb.open() as app:
        assert app.widget('schematicEditor').properties()['visible'] is True
        assert app.widget('boardEditor').properties()['visible'] is True
