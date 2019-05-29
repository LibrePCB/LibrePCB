#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test creating a device with the library editor
"""


def test(library_editor, helpers):
    """
    Create new device
    """
    le = library_editor

    # Open "New Library Element" wizard
    le.action('libraryEditorActionNewElement').trigger(blocking=False)

    # Choose type of element
    le.widget('libraryEditorNewElementWizardChooseTypeDeviceButton').click()

    # Enter metadata
    widget_properties = {
        ('NameEdit', 'text'): 'New Device',
        ('DescriptionEdit', 'plainText'): 'Foo Bar',
        ('KeywordsEdit', 'text'): '',
        ('AuthorEdit', 'text'): 'Functional Test',
        ('VersionEdit', 'text'): '1.2.3',
    }
    for (widget, property), value in widget_properties.items():
        le.widget('libraryEditorNewElementWizardMetadata' + widget).set_property(property, value)
    le.widget('libraryEditorNewElementWizardNextButton').click()

    # Select component
    le.widget('libraryEditorNewElementWizardDevicePropertiesChooseComponentButton').click()
    category_tree = le.widget('libraryEditorNewElementWizardDevicePropertiesChooseComponentDialogCategoriesTree')
    helpers.wait_for_model_items_count(category_tree, 1)
    category = category_tree.model().items().items[0]
    category_tree.select_item(category)
    components_list = le.widget('libraryEditorNewElementWizardDevicePropertiesChooseComponentDialogComponentsList')
    helpers.wait_for_model_items_count(components_list, 1)
    component = components_list.model().items().items[0]
    components_list.select_item(component)
    le.widget('libraryEditorNewElementWizardDevicePropertiesChooseComponentDialogAcceptButton').click()

    # Select package
    le.widget('libraryEditorNewElementWizardDevicePropertiesChoosePackageButton').click()
    category_tree = le.widget('libraryEditorNewElementWizardDevicePropertiesChoosePackageDialogCategoriesTree')
    helpers.wait_for_model_items_count(category_tree, 3)
    category = category_tree.model().items().items[2]
    category_tree.select_item(category)
    packages_list = le.widget('libraryEditorNewElementWizardDevicePropertiesChoosePackageDialogPackagesList')
    helpers.wait_for_model_items_count(packages_list, 1)
    package = packages_list.model().items().items[0]
    packages_list.select_item(package)
    le.widget('libraryEditorNewElementWizardDevicePropertiesChoosePackageDialogAcceptButton').click()

    # Finish
    dialog = le.widget('libraryEditorNewElementWizard')
    le.widget('libraryEditorNewElementWizardFinishButton').click()
    helpers.wait_until_widget_hidden(dialog)

    # Check if a new tab is opened (indicates that the element was created)
    tab_props = le.widget('libraryEditorStackedWidget').properties()
    assert tab_props['count'] == 2
    assert tab_props['currentIndex'] == 1

    # Check metadata
    assert le.widget('libraryEditorDeviceNameEdit').properties()['text'] == 'New Device'
    assert le.widget('libraryEditorDeviceDescriptionEdit').properties()['plainText'] == 'Foo Bar'
    assert le.widget('libraryEditorDeviceKeywordsEdit').properties()['text'] == ''
    assert le.widget('libraryEditorDeviceAuthorEdit').properties()['text'] == 'Functional Test'
    assert le.widget('libraryEditorDeviceVersionEdit').properties()['text'] == '1.2.3'
