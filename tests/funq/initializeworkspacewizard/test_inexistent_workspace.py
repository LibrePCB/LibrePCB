#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the "Initialize Workspace Wizard" on an existing installation, i.e.
but with an invalid (inexistent) workspace set in the user settings
"""

import os


def test_create_new_workspace(librepcb, helpers):
    """
    Choose a non-existent workspace path to create a new workspace
    """
    librepcb.workspace_path = librepcb.workspace_path + '-test'
    with librepcb.open() as app:
        # Choose workspace path.
        wizard = app.widget('initWorkspaceWizard')
        path_edit = app.widget('initWorkspaceWizardChooseWorkspacePathEdit')
        assert path_edit.properties()['text'] == librepcb.workspace_path
        status_label = app.widget('initWorkspaceWizardChooseWorkspaceStatusLabel')
        assert 'workspace will be created' in status_label.properties()['text']
        app.widget('initWorkspaceWizardNextButton').click()

        # Choose workspace settings.
        user_edit = app.widget('initWorkspaceWizardChooseSettingsUserNameEdit')
        user_edit.set_property('text', 'foobar 42')
        app.widget('initWorkspaceWizardFinishButton').click()

        # Verify that the workspace has been created.
        helpers.wait_until_widget_hidden(wizard)
        assert os.path.exists(os.path.join(librepcb.workspace_path,
                                           '.librepcb-workspace'))

        # Verify that the control panel is now opened.
        app.widget('controlPanel').properties()['visible'] is True

    # Open LibrePCB again to see if the workspace is automatically opened
    # and the settings are applied.
    with librepcb.open() as app:
        app.widget('controlPanel').properties()['visible'] is True
        app.action('controlPanelActionOpenWorkspaceSettings').trigger(blocking=False)
        user_edit = app.widget('controlPanelWorkspaceSettingsDialogGeneralUserNameEdit')
        assert user_edit.properties()['text'] == 'foobar 42'


def test_open_compatible_workspace(librepcb):
    """
    Choose a compatible workspace to open
    """
    existing_workspace_path = librepcb.workspace_path
    librepcb.workspace_path = librepcb.workspace_path + '-test'
    with librepcb.open() as app:
        # Choose workspace path.
        path_edit = app.widget('initWorkspaceWizardChooseWorkspacePathEdit')
        assert path_edit.properties()['text'] == librepcb.workspace_path
        status_label = app.widget('initWorkspaceWizardChooseWorkspaceStatusLabel')
        assert 'workspace will be created' in status_label.properties()['text']
        path_edit.set_property('text', existing_workspace_path)
        assert 'contains a valid workspace' in status_label.properties()['text']
        app.widget('initWorkspaceWizardFinishButton').click()

        # Verify that the control panel is now opened.
        app.widget('controlPanel').properties()['visible'] is True

    # Open LibrePCB again to see if the workspace is automatically opened.
    with librepcb.open() as app:
        app.widget('controlPanel').properties()['visible'] is True
