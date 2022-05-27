#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the "Initialize Workspace Wizard" on an existing installation, i.e.
with a workspace already set in the user settings
"""

import os
import shutil


def test_downgrade(librepcb, helpers):
    """
    Test opening a workspace which needs to be initialized with an
    older file format
    """
    v01_path = os.path.join(librepcb.workspace_path, 'v0.1')
    data_path = os.path.join(librepcb.workspace_path, 'data')
    shutil.move(v01_path, data_path)
    with open(os.path.join(data_path, '.librepcb-data'), 'wb') as f:
        f.write(b'999\n')
    with librepcb.open() as app:
        # Choose workspace settings.
        wizard = app.widget('initWorkspaceWizard')
        user_edit = app.widget('initWorkspaceWizardChooseSettingsUserNameEdit')
        user_edit.set_property('text', 'foobar 42')
        app.widget('initWorkspaceWizardFinishButton').click()

        # Verify that the v0.1 directory has been created.
        helpers.wait_until_widget_hidden(wizard)
        assert os.path.exists(v01_path)
        assert not os.path.exists(os.path.join(v01_path, '.librepcb-data'))

        # Verify that the control panel is now opened.
        app.widget('controlPanel').properties()['visible'] is True

    # Open LibrePCB again to see if the workspace is automatically opened.
    with librepcb.open() as app:
        app.widget('controlPanel').properties()['visible'] is True
