#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test creating projects
"""

import os


def test_new_project_wizard(librepcb, helpers):
    """
    Create project using the wizard from the main window
    """
    with librepcb.open() as app:
        adapter = app.widget('mainWindowTestAdapter', wait_active=True)
        # Open new project wizard
        adapter.call_slot('trigger', 'project-new')
        assert app.widget('newProjectWizard').properties()['visible'] is True
        # Enter metadata
        name = 'New Project'
        app.widget('newProjectWizardMetadataNameEdit').set_property('text', name)
        path = app.widget('newProjectWizardMetadataPathEdit').properties()['text']
        app.widget('newProjectWizardNextButton').click()
        # Setup schematic/board
        app.widget('newProjectWizardFinishButton').click()
        # Verify that the project was opened
        helpers.wait_for_project(app, 'New Project')
        assert os.path.exists(path)

    # Open project again to see if it was saved properly
    librepcb.set_project(path)
    with librepcb.open() as app:
        helpers.wait_for_project(app, 'New Project')
