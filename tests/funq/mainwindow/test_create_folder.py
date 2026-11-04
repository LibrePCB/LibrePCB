#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test creating folders in the workspace projects directory
"""

import os


def test_new_root_folder(librepcb, helpers):
    """
    Create a new folder in the workspace projects root directory by triggering
    the corresponding action of the home panel
    """
    with librepcb.open() as app:
        adapter = app.widget("mainWindowTestAdapter", wait_active=True)
        # Open new folder dialog
        adapter.call_slot("trigger", "workspace-new-root-folder")
        dialog = app.widget("mainWindowNewFolderDialog")
        helpers.wait_for_active_dialog(app, dialog)
        # Verify that the folder doesn't exist yet
        name = "New Folder"
        path = os.path.join(librepcb.workspace_path, "projects", name)
        assert not os.path.isdir(path)
        # Enter folder name and confirm
        app.widget("mainWindowNewFolderDialogNameEdit").set_property("text", name)
        app.widget("mainWindowNewFolderDialogOkButton").click()
        helpers.wait_until_widget_hidden(dialog)  # raises on timeout
        # Verify that the folder was created
        assert os.path.isdir(path)
