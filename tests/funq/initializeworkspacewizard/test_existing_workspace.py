#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the "Initialize Workspace Wizard" on an existing installation, i.e.
with a workspace already set in the user settings
"""

import os
import pytest
import shutil
import sys


@pytest.mark.xfail(sys.platform == 'darwin', raises=AssertionError,
                   strict=False, reason="Flaky on macOS, no idea why.")
def test_upgrade_v01(librepcb, helpers):
    """
    Test an upgrade which copies from 'v0.1' to 'data'
    """
    v01_path = os.path.join(librepcb.workspace_path, 'v0.1')
    data_path = os.path.join(librepcb.workspace_path, 'data')
    shutil.rmtree(data_path)
    with librepcb.open() as app:
        # Perform upgrade.
        wizard = app.widget('initWorkspaceWizard')
        src_label = app.widget('initWorkspaceWizardUpgradeSourceLabel')
        assert v01_path in src_label.properties()['text']
        dst_label = app.widget('initWorkspaceWizardUpgradeDestinationLabel')
        assert data_path in dst_label.properties()['text']
        app.widget('initWorkspaceWizardFinishButton').click()

        # Verify that the data directory has been created.
        helpers.wait_until_widget_hidden(wizard)
        assert os.path.exists(os.path.join(data_path, '.librepcb-data'))

        # Verify that the main window is now opened.
        app.widget('mainWindow').properties()['visible'] is True

    # Open LibrePCB again to see if the workspace is automatically opened.
    with librepcb.open() as app:
        app.widget('mainWindow').properties()['visible'] is True


@pytest.mark.xfail(sys.platform == 'darwin', raises=AssertionError,
                   strict=False, reason="Flaky on macOS, no idea why.")
def test_upgrade_data(librepcb, helpers):
    """
    Test an upgrade which copies from 'data' to 'v0.1'
    """
    v01_path = os.path.join(librepcb.workspace_path, 'v0.1')
    data_path = os.path.join(librepcb.workspace_path, 'data')
    shutil.rmtree(v01_path)
    os.remove(os.path.join(data_path, '.librepcb-data'))  # Downgrade to v0.1
    with librepcb.open() as app:
        # Perform upgrade.
        wizard = app.widget('initWorkspaceWizard')
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

        # Verify that the main window is now opened.
        app.widget('mainWindow').properties()['visible'] is True

    # Open LibrePCB again to see if the workspace is automatically opened.
    with librepcb.open() as app:
        app.widget('mainWindow').properties()['visible'] is True


def test_downgrade(librepcb, helpers):
    """
    Test opening a workspace which needs to be initialized with an
    older file format
    """
    v01_path = os.path.join(librepcb.workspace_path, 'v0.1')
    v02_path = os.path.join(librepcb.workspace_path, 'v1')
    data_path = os.path.join(librepcb.workspace_path, 'data')
    shutil.rmtree(v01_path)
    with open(os.path.join(data_path, '.librepcb-data'), 'wb') as f:
        f.write(b'999\n')
    with librepcb.open() as app:
        # Choose workspace settings.
        wizard = app.widget('initWorkspaceWizard')
        user_edit = app.widget('initWorkspaceWizardChooseSettingsUserNameEdit')
        user_edit.set_property('text', 'foobar 42')
        app.widget('initWorkspaceWizardFinishButton').click()

        # Verify that the v1 directory has been created.
        helpers.wait_until_widget_hidden(wizard)
        assert os.path.exists(os.path.join(v02_path, '.librepcb-data'))

        # Verify that the main window is now opened.
        app.widget('mainWindow').properties()['visible'] is True

    # Open LibrePCB again to see if the workspace is automatically opened.
    with librepcb.open() as app:
        app.widget('mainWindow').properties()['visible'] is True
