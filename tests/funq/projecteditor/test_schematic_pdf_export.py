#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pytest
import os

"""
Test PDF export dialog in schematic editor
"""


@pytest.fixture
def dialog(project_editor):
    """
    Fixture opening the PDF export dialog in the schematic editor
    """
    adapter = project_editor.widget('mainWindowTestAdapter', wait_active=True)
    adapter.call_slot('trigger', 'schematic-export-pdf-dialog')
    dialog = project_editor.widget('graphicsExportDialog')

    # Do not open exported files since this would be annoying here.
    project_editor.widget(
        'graphicsExportDialogOpenFileCheckbox').\
        set_property('checked', False)

    yield dialog


def test_active_widget(dialog, project_editor, helpers):
    """
    Test if the dialog is the active widget after opening it
    """
    helpers.wait_for_active_dialog(project_editor, dialog)  # raises on timeout


def test_cancel_by_button(dialog, project_editor, helpers):
    """
    Test if the cancel button closes the dialog
    """
    project_editor.widget(
        'graphicsExportDialogButtonCancel').click()
    helpers.wait_until_widget_hidden(dialog)  # raises on timeout


def test_cancel_by_escape(dialog, project_editor, helpers):
    """
    Test if the escape key closes the dialog
    """
    dialog.shortcut('Escape')
    helpers.wait_until_widget_hidden(dialog)  # raises on timeout


def test_restore_defaults(dialog, project_editor, librepcb, helpers):
    """
    Test if the restore defaults button resets the settings
    """
    cbx = project_editor.widget(
        'graphicsExportDialogPageSizeComboBox')
    index = cbx.properties()['currentIndex']
    cbx.set_property('currentIndex', index + 1)
    assert cbx.properties()['currentIndex'] == index + 1
    project_editor.widget(
        'graphicsExportDialogButtonRestoreDefaults').click()
    assert cbx.properties()['currentIndex'] == index


def test_export(dialog, project_editor, librepcb, helpers):
    """
    Test if the accept button asks for a filepath and then creates the PDF file
    """
    project_editor.widget(
        'graphicsExportDialogButtonAccept').click()
    path = librepcb.abspath('test export.pdf')
    project_editor.widget(
        'graphicsExportSaveFileDialogFileNameEdit').\
        set_property('text', path)
    project_editor.widget(
        'graphicsExportSaveFileDialogOkButton').click()
    helpers.wait_until_widget_hidden(dialog)  # raises on timeout
    assert os.path.exists(path)
