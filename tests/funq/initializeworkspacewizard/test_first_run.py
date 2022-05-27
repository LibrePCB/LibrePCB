#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the "Initialize Workspace Wizard" on a fresh installation, i.e.
without any workspace set in the user settings
"""

import os
import shutil


def test_env_default_path(librepcb, helpers):
    """
    Set the LIBREPCB_DEFAULT_WORKSPACE_PATH environment variable and check
    if the default workspace path is set to this value
    """
    test_workspace_path = librepcb.workspace_path
    librepcb.env['LIBREPCB_DEFAULT_WORKSPACE_PATH'] = test_workspace_path
    librepcb.workspace_path = None  # Do not set workspace in LibrePCB.ini
    with librepcb.open() as app:
        # Skip "Welcome to LibrePCB" page.
        app.widget('initWorkspaceWizardNextButton').click()

        # Check workspace path.
        path_edit = app.widget('initWorkspaceWizardChooseWorkspacePathEdit')
        assert path_edit.properties()['text'] == test_workspace_path
        status_label = app.widget('initWorkspaceWizardChooseWorkspaceStatusLabel')
        assert 'contains a valid workspace' in status_label.properties()['text']


def test_create_new_workspace(librepcb, helpers):
    """
    Choose a non-existent workspace path to create a new workspace
    """
    test_workspace_path = librepcb.workspace_path + '-test'
    librepcb.workspace_path = None  # Do not set workspace in LibrePCB.ini
    with librepcb.open() as app:
        # Skip "Welcome to LibrePCB" page.
        wizard = app.widget('initWorkspaceWizard')
        app.widget('initWorkspaceWizardNextButton').click()

        # Choose workspace path.
        path_edit = app.widget('initWorkspaceWizardChooseWorkspacePathEdit')
        default_path = os.path.join(os.path.expanduser('~'), 'LibrePCB-Workspace')
        assert path_edit.properties()['text'] == default_path
        path_edit.set_property('text', test_workspace_path)
        status_label = app.widget('initWorkspaceWizardChooseWorkspaceStatusLabel')
        assert 'workspace will be created' in status_label.properties()['text']
        app.widget('initWorkspaceWizardNextButton').click()

        # Choose workspace settings.
        user_edit = app.widget('initWorkspaceWizardChooseSettingsUserNameEdit')
        user_edit.set_property('text', 'foobar 42')
        app.widget('initWorkspaceWizardFinishButton').click()

        # Verify that the workspace has been created.
        helpers.wait_until_widget_hidden(wizard)
        assert os.path.exists(os.path.join(test_workspace_path,
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
    test_workspace_path = librepcb.workspace_path
    librepcb.workspace_path = None  # Do not set workspace in LibrePCB.ini
    with librepcb.open() as app:
        # Skip "Welcome to LibrePCB" page.
        app.widget('initWorkspaceWizardNextButton').click()

        # Choose workspace path.
        path_edit = app.widget('initWorkspaceWizardChooseWorkspacePathEdit')
        default_path = os.path.join(os.path.expanduser('~'), 'LibrePCB-Workspace')
        assert path_edit.properties()['text'] == default_path
        path_edit.set_property('text', test_workspace_path)
        status_label = app.widget('initWorkspaceWizardChooseWorkspaceStatusLabel')
        assert 'contains a valid workspace' in status_label.properties()['text']
        app.widget('initWorkspaceWizardFinishButton').click()

        # Verify that the control panel is now opened.
        app.widget('controlPanel').properties()['visible'] is True

    # Open LibrePCB again to see if the workspace is automatically opened.
    with librepcb.open() as app:
        app.widget('controlPanel').properties()['visible'] is True


def test_open_workspace_upgrade_v01(librepcb, helpers):
    """
    Choose a workspace to open, which needs to be upgraded first, by
    copying 'v0.1' to 'data'
    """
    test_workspace_path = librepcb.workspace_path
    librepcb.workspace_path = None  # Do not set workspace in LibrePCB.ini
    v01_path = os.path.join(test_workspace_path, 'v0.1')
    data_path = os.path.join(test_workspace_path, 'data')
    shutil.rmtree(data_path)
    with librepcb.open() as app:
        # Skip "Welcome to LibrePCB" page.
        wizard = app.widget('initWorkspaceWizard')
        app.widget('initWorkspaceWizardNextButton').click()

        # Choose workspace path.
        path_edit = app.widget('initWorkspaceWizardChooseWorkspacePathEdit')
        default_path = os.path.join(os.path.expanduser('~'), 'LibrePCB-Workspace')
        assert path_edit.properties()['text'] == default_path
        path_edit.set_property('text', test_workspace_path)
        status_label = app.widget('initWorkspaceWizardChooseWorkspaceStatusLabel')
        assert 'contains a valid workspace' in status_label.properties()['text']
        app.widget('initWorkspaceWizardNextButton').click()

        # Perform upgrade.
        src_label = app.widget('initWorkspaceWizardUpgradeSourceLabel')
        assert v01_path in src_label.properties()['text']
        dst_label = app.widget('initWorkspaceWizardUpgradeDestinationLabel')
        assert data_path in dst_label.properties()['text']
        app.widget('initWorkspaceWizardFinishButton').click()

        # Verify that the data directory has been created.
        helpers.wait_until_widget_hidden(wizard)
        assert os.path.exists(os.path.join(data_path, '.librepcb-data'))

        # Verify that the control panel is now opened.
        app.widget('controlPanel').properties()['visible'] is True

    # Open LibrePCB again to see if the workspace is automatically opened.
    with librepcb.open() as app:
        app.widget('controlPanel').properties()['visible'] is True


def test_open_workspace_upgrade_data(librepcb, helpers):
    """
    Choose a workspace to open, which needs to be upgraded first, by
    copying 'data' to 'v0.1'
    """
    test_workspace_path = librepcb.workspace_path
    librepcb.workspace_path = None  # Do not set workspace in LibrePCB.ini
    v01_path = os.path.join(test_workspace_path, 'v0.1')
    data_path = os.path.join(test_workspace_path, 'data')
    shutil.rmtree(v01_path)
    os.remove(os.path.join(data_path, '.librepcb-data'))  # Downgrade to v0.1
    with librepcb.open() as app:
        # Skip "Welcome to LibrePCB" page.
        wizard = app.widget('initWorkspaceWizard')
        app.widget('initWorkspaceWizardNextButton').click()

        # Choose workspace path.
        path_edit = app.widget('initWorkspaceWizardChooseWorkspacePathEdit')
        default_path = os.path.join(os.path.expanduser('~'), 'LibrePCB-Workspace')
        assert path_edit.properties()['text'] == default_path
        path_edit.set_property('text', test_workspace_path)
        status_label = app.widget('initWorkspaceWizardChooseWorkspaceStatusLabel')
        assert 'contains a valid workspace' in status_label.properties()['text']
        app.widget('initWorkspaceWizardNextButton').click()

        # Perform upgrade.
        src_label = app.widget('initWorkspaceWizardUpgradeSourceLabel')
        assert data_path in src_label.properties()['text']
        dst_label = app.widget('initWorkspaceWizardUpgradeDestinationLabel')
        assert v01_path in dst_label.properties()['text']
        app.widget('initWorkspaceWizardFinishButton').click()

        # Verify that the v0.1 directory has been created and the data
        # directory has been upgraded.
        helpers.wait_until_widget_hidden(wizard)
        assert os.path.exists(v01_path)
        assert os.path.exists(os.path.join(data_path, '.librepcb-data'))

        # Verify that the control panel is now opened.
        app.widget('controlPanel').properties()['visible'] is True

    # Open LibrePCB again to see if the workspace is automatically opened.
    with librepcb.open() as app:
        app.widget('controlPanel').properties()['visible'] is True


def test_open_workspace_downgrade(librepcb, helpers):
    """
    Choose a workspace to open, which needs to be initialized with an
    older file format
    """
    test_workspace_path = librepcb.workspace_path
    librepcb.workspace_path = None  # Do not set workspace in LibrePCB.ini
    v01_path = os.path.join(test_workspace_path, 'v0.1')
    v02_path = os.path.join(test_workspace_path, 'v0.2')
    data_path = os.path.join(test_workspace_path, 'data')
    shutil.rmtree(v01_path)
    with open(os.path.join(data_path, '.librepcb-data'), 'wb') as f:
        f.write(b'999\n')
    with librepcb.open() as app:
        # Skip "Welcome to LibrePCB" page.
        wizard = app.widget('initWorkspaceWizard')
        app.widget('initWorkspaceWizardNextButton').click()

        # Choose workspace path.
        path_edit = app.widget('initWorkspaceWizardChooseWorkspacePathEdit')
        default_path = os.path.join(os.path.expanduser('~'), 'LibrePCB-Workspace')
        assert path_edit.properties()['text'] == default_path
        path_edit.set_property('text', test_workspace_path)
        status_label = app.widget('initWorkspaceWizardChooseWorkspaceStatusLabel')
        assert 'contains a valid workspace' in status_label.properties()['text']
        app.widget('initWorkspaceWizardNextButton').click()

        # Choose workspace settings.
        user_edit = app.widget('initWorkspaceWizardChooseSettingsUserNameEdit')
        user_edit.set_property('text', 'foobar 42')
        app.widget('initWorkspaceWizardFinishButton').click()

        # Verify that the v0.2 directory has been created.
        helpers.wait_until_widget_hidden(wizard)
        assert os.path.exists(os.path.join(v02_path, '.librepcb-data'))

        # Verify that the control panel is now opened.
        app.widget('controlPanel').properties()['visible'] is True

    # Open LibrePCB again to see if the workspace is automatically opened.
    with librepcb.open() as app:
        app.widget('controlPanel').properties()['visible'] is True
