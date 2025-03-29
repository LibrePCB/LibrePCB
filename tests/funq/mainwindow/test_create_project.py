#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test creating projects
"""

import os


def test_new_project_wizard(librepcb):
    """
    Create project using the wizard from the main window
    """
    with librepcb.open() as app:
        # Open new project wizard
        app.widget('mainWindowTestAdapter').call_slot('trigger', 'project-new')
        assert app.widget('mainWindowNewProjectWizard').properties()['visible'] is True
        # Enter metadata
        name = 'New Project'
        app.widget('mainWindowNewProjectWizardMetadataNameEdit').set_property('text', name)
        path = app.widget('mainWindowNewProjectWizardMetadataPathEdit').properties()['text']
        app.widget('mainWindowNewProjectWizardNextButton').click()
        # Setup schematic/board
        app.widget('mainWindowNewProjectWizardFinishButton').click()
        # Verify if editors are opened and project file exists
        assert app.widget('schematicEditor').properties()['visible'] is True
        assert app.widget('boardEditor').properties()['visible'] is True
        assert os.path.exists(path)

    # Open project again to see if it was saved properly
    librepcb.set_project(path)
    with librepcb.open() as app:
        assert app.widget('schematicEditor').properties()['visible'] is True
        assert app.widget('boardEditor').properties()['visible'] is True
