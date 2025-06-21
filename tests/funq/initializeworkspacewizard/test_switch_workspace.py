#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the "Initialize Workspace Wizard" in the main window to switch to
a different workspace
"""

import os
import shutil


def test_create_new_workspace(librepcb, helpers):
    """
    Choose a non-existent workspace path to create a new workspace
    """
    test_workspace_path = librepcb.workspace_path + "-test"
    with librepcb.open() as app:
        # Open "Switch Workspace" dialog.
        app.widget("mainWindow").properties()["visible"] is True
        app.widget("mainWindowTestAdapter").call_slot("trigger", "workspace-switch")
        wizard = app.widget("initWorkspaceWizard")

        # Choose workspace path.
        path_edit = app.widget("initWorkspaceWizardChooseWorkspacePathEdit")
        assert path_edit.properties()["text"] == librepcb.workspace_path
        status_label = app.widget("initWorkspaceWizardChooseWorkspaceStatusLabel")
        assert "contains a valid workspace" in status_label.properties()["text"]
        path_edit.set_property("text", test_workspace_path)
        assert "workspace will be created" in status_label.properties()["text"]
        app.widget("initWorkspaceWizardNextButton").click()

        # Choose workspace settings.
        user_edit = app.widget("initWorkspaceWizardChooseSettingsUserNameEdit")
        user_edit.set_property("text", "foobar 1337")
        app.widget("initWorkspaceWizardFinishButton").click()

        # Verify that the workspace has been created.
        helpers.wait_until_widget_hidden(wizard)
        assert os.path.exists(os.path.join(test_workspace_path, ".librepcb-workspace"))

    # Open LibrePCB again to see if the workspace is automatically opened
    # and the settings are applied.
    with librepcb.open() as app:
        app.widget("mainWindow").properties()["visible"] is True
        app.widget("mainWindowTestAdapter").call_slot("trigger", "workspace-settings")
        user_edit = app.widget("workspaceSettingsDialogGeneralUserNameEdit")
        assert user_edit.properties()["text"] == "foobar 1337"


def test_open_compatible_workspace(librepcb, helpers):
    """
    Choose a compatible workspace to open
    """
    test_workspace_path = librepcb.workspace_path + "-test"
    shutil.copytree(librepcb.workspace_path, test_workspace_path)
    with librepcb.open() as app:
        # Open "Switch Workspace" dialog.
        app.widget("mainWindow").properties()["visible"] is True
        app.widget("mainWindowTestAdapter").call_slot("trigger", "workspace-switch")
        wizard = app.widget("initWorkspaceWizard")

        # Choose workspace path.
        path_edit = app.widget("initWorkspaceWizardChooseWorkspacePathEdit")
        assert path_edit.properties()["text"] == librepcb.workspace_path
        status_label = app.widget("initWorkspaceWizardChooseWorkspaceStatusLabel")
        assert "contains a valid workspace" in status_label.properties()["text"]
        path_edit.set_property("text", test_workspace_path)
        assert "contains a valid workspace" in status_label.properties()["text"]
        app.widget("initWorkspaceWizardFinishButton").click()
        helpers.wait_until_widget_hidden(wizard)

    # Open LibrePCB again to see if the workspace is automatically opened.
    shutil.rmtree(librepcb.workspace_path)
    with librepcb.open() as app:
        app.widget("mainWindow").properties()["visible"] is True
