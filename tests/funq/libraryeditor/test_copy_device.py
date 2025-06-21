#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test copying a device in the library editor
"""


def test(library_editor, helpers):
    """
    Copy device with "New Library Element" wizard
    """
    le = library_editor

    # Open "New Library Element" wizard
    le.action("libraryEditorActionNewElement").trigger(blocking=False)

    # Choose "Copy existing element"
    le.widget("libraryEditorNewElementWizardChooseTypeCopyRadioButton").set_property(
        "checked", True
    )

    # Choose type of element
    le.widget("libraryEditorNewElementWizardChooseTypeDeviceButton").click()

    # Choose category
    category_tree = le.widget("libraryEditorNewElementWizardCopyFromCategoriesTree")
    helpers.wait_for_model_items_count(category_tree, 1)
    category = category_tree.model().items().items[0]
    category_tree.select_item(category)

    # Choose element
    element_list = le.widget("libraryEditorNewElementWizardCopyFromElementsList")
    helpers.wait_for_model_items_count(element_list, 1)
    element = element_list.model().items().items[0]
    element_list.select_item(element)
    le.widget("libraryEditorNewElementWizardNextButton").click()

    # Check metadata
    widget_properties = {
        ("NameEdit", "text"): "C-0805",
        ("DescriptionEdit", "plainText"): "",
        ("KeywordsEdit", "text"): "",
        ("VersionEdit", "text"): "0.1",
    }
    for (widget, property), value in widget_properties.items():
        props = le.widget("libraryEditorNewElementWizardMetadata" + widget).properties()
        assert props[property] == value
    le.widget("libraryEditorNewElementWizardNextButton").click()

    # Check device properties
    component_name_label = le.widget(
        "libraryEditorNewElementWizardDevicePropertiesComponentNameLabel"
    )
    assert component_name_label.properties()["text"] == "Capacitor Bipolar"
    package_name_label = le.widget(
        "libraryEditorNewElementWizardDevicePropertiesPackageNameLabel"
    )
    assert package_name_label.properties()["text"] == "C-0805"

    # Finish
    dialog = le.widget("libraryEditorNewElementWizard")
    le.widget("libraryEditorNewElementWizardFinishButton").click()
    helpers.wait_until_widget_hidden(dialog)

    # Check if a new tab is opened (indicates that the element was created)
    tab_props = le.widget("libraryEditorStackedWidget").properties()
    assert tab_props["count"] == 2
    assert tab_props["currentIndex"] == 1

    # Check metadata
    assert le.widget("libraryEditorDeviceNameEdit").properties()["text"] == "C-0805"
    assert (
        le.widget("libraryEditorDeviceDescriptionEdit").properties()["plainText"] == ""
    )
    assert le.widget("libraryEditorDeviceKeywordsEdit").properties()["text"] == ""
    assert le.widget("libraryEditorDeviceVersionEdit").properties()["text"] == "0.1"
