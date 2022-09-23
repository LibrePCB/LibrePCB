#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pytest
import os

"""
Test image export dialog in schematic editor

Meant as an addition to the tests in test_schematic_pdf_export.py just to
test the image export functionality as well.
"""


@pytest.fixture
def dialog(project_editor):
    """
    Fixture opening the image export dialog in the schematic editor
    """
    project_editor.action('schematicEditorActionExportImage').\
        trigger(blocking=False)
    dialog = project_editor.widget('schematicEditorGraphicsExportDialog')

    # Do not open exported files since this would be annoying here.
    project_editor.widget(
        'schematicEditorGraphicsExportDialogOpenFileCheckbox').\
        set_property('checked', False)

    yield dialog


def test_export_png(dialog, project_editor, librepcb, helpers):
    """
    Test if the accept button asks for a filepath and then creates a PNG file
    """
    project_editor.widget(
        'schematicEditorGraphicsExportDialogButtonAccept').click()
    path = librepcb.abspath('test export.png')
    project_editor.widget(
        'schematicEditorGraphicsExportSaveFileDialogFileNameEdit').\
        set_property('text', path)
    project_editor.widget(
        'schematicEditorGraphicsExportSaveFileDialogOkButton').click()
    helpers.wait_until_widget_hidden(dialog)  # raises on timeout
    assert os.path.exists(path)
