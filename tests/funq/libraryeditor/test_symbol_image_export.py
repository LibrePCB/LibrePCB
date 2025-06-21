#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pytest
import os

"""
Test image export dialog in symbol editor

Meant as an addition to the tests in test_schematic_image_export.py just to
test the symbol editor specific functionality works as well.
"""


@pytest.fixture
def dialog(library_editor, helpers):
    """
    Fixture opening the image export dialog in the symbol editor
    """
    list_widget = library_editor.widget("libraryEditorOverviewSymList")
    helpers.wait_for_model_items_count(list_widget, 1)
    item = list_widget.model().items().items[0]
    list_widget.dclick_item(item)

    library_editor.action("libraryEditorActionExportImage").trigger(blocking=False)
    dialog = library_editor.widget("libraryEditorGraphicsExportDialog")

    # Do not open exported files since this would be annoying here.
    library_editor.widget(
        "libraryEditorGraphicsExportDialogOpenFileCheckbox"
    ).set_property("checked", False)

    yield dialog


def test_export_png(dialog, library_editor, librepcb, helpers):
    """
    Test if the accept button asks for a filepath and then creates a PNG file
    """
    library_editor.widget("libraryEditorGraphicsExportDialogButtonAccept").click()
    path = librepcb.abspath("test export.png")
    library_editor.widget(
        "libraryEditorGraphicsExportSaveFileDialogFileNameEdit"
    ).set_property("text", path)
    library_editor.widget("libraryEditorGraphicsExportSaveFileDialogOkButton").click()
    helpers.wait_until_widget_hidden(dialog)  # raises on timeout
    assert os.path.exists(path)
