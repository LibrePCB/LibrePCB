#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test importing an EAGLE library with the library editor
"""


def _get_elements_count(le):
    symbols = len(le.widget("libraryEditorOverviewSymList").model().items().items)
    packages = len(le.widget("libraryEditorOverviewPkgList").model().items().items)
    components = len(le.widget("libraryEditorOverviewCmpList").model().items().items)
    devices = len(le.widget("libraryEditorOverviewDevList").model().items().items)
    return symbols, packages, components, devices


def test(librepcb, library_editor, helpers):
    """
    Import EAGLE library
    """
    le = library_editor
    lbr = librepcb.get_data_path("unittests/eagleimport/resistor.lbr")

    # Get current library elements count
    symbols, packages, components, devices = _get_elements_count(le)

    # Open "EAGLE Import" wizard
    le.action("libraryEditorActionImportEagleLibrary").trigger(blocking=False)
    next_button = le.widget("libraryEditorEagleLibraryImportWizardNextButton")

    # Choose library - first an invalid path, then a valid path
    next_button.click()
    path_edit = le.widget(
        "libraryEditorEagleLibraryImportWizardChooseLibraryFilePathEdit"
    )
    assert next_button.properties()["enabled"] is False
    path_edit.set_property("text", lbr + "_")
    assert next_button.properties()["enabled"] is False
    path_edit.set_property("text", lbr)
    assert next_button.properties()["enabled"] is True

    # Choose elements to import
    next_button.click()
    tree_widget = le.widget(
        "libraryEditorEagleLibraryImportWizardSelectElementsTreeWidget"
    )
    helpers.wait_for_model_items_count(tree_widget, 4, 4)
    component = tree_widget.model().items().items[0]
    tree_widget.select_item(component)
    assert next_button.properties()["enabled"] is False
    tree_widget.shortcut("Space")  # Select item
    assert next_button.properties()["enabled"] is True
    tree_widget.shortcut("Space")  # Deselect item
    assert next_button.properties()["enabled"] is False
    tree_widget.shortcut("Space")  # Select item
    assert next_button.properties()["enabled"] is True

    # Set import options
    next_button.click()
    add_prefix_checkbox = le.widget(
        "libraryEditorEagleLibraryImportWizardSetOptionsPrefixCheckBox"
    )
    add_prefix_checkbox.click()
    choose_cmpcat_button = le.widget(
        "libraryEditorEagleLibraryImportWizardSetOptionsComponentCategoryChooseButton"
    )
    choose_cmpcat_button.click()
    category_tree = le.widget(
        "libraryEditorEagleLibraryImportWizardSetOptionsChooseCategoryTree"
    )
    helpers.wait_for_model_items_count(category_tree, 1)
    category = category_tree.model().items().items[0]
    category_tree.select_item(category)
    category_tree.shortcut("Return")

    # Import
    import_button = le.widget("libraryEditorEagleLibraryImportWizardImportButton")
    import_button.click()
    finish_button = le.widget("libraryEditorEagleLibraryImportWizardFinishButton")
    finish_button.wait_for_properties(dict(enabled=True))
    finish_button.click()

    # Check if elements were added to library
    helpers.wait_for_library_scan_complete(le)
    assert _get_elements_count(le) == (
        symbols + 1,
        packages + 1,
        components + 1,
        devices + 1,
    )
