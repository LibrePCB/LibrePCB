#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test copying a device in the library editor
"""


def test(library_editor, helpers):
    """
    Copy device with "New Library Element" wizard
    """

    # Open "New Library Element" wizard
    library_editor.action('libraryEditorActionNewElement').trigger(blocking=False)

    # Choose "Copy existing element"
    library_editor.widget('libraryEditorNewElementWizardChooseTypeCopyRadioButton').set_property('checked', True)

    # Choose type of element
    library_editor.widget('libraryEditorNewElementWizardChooseTypeDeviceButton').click()

    # Choose category
    category_tree = library_editor.widget('libraryEditorNewElementWizardCopyFromCategoriesTree')
    helpers.wait_for_model_items_count(category_tree, 1)
    category_tree.model_items().items[0].select()

    # Choose element
    element_list = library_editor.widget('libraryEditorNewElementWizardCopyFromElementsList')
    helpers.wait_for_model_items_count(element_list, 1)
    element_list.model_items().items[0].select()
    library_editor.widget('libraryEditorNewElementWizardNextButton').click()

    # Check metadata
    widget_properties = {
        ('NameEdit', 'text'): 'C-0805',
        ('DescriptionEdit', 'plainText'): '',
        ('KeywordsEdit', 'text'): '',
        ('VersionEdit', 'text'): '0.1',
    }
    for (widget, property), value in widget_properties.items():
        props = library_editor.widget('libraryEditorNewElementWizardMetadata' + widget).properties()
        assert props[property] == value
    library_editor.widget('libraryEditorNewElementWizardNextButton').click()

    # Check device properties
    component_name_label = library_editor.widget('libraryEditorNewElementWizardDevicePropertiesComponentNameLabel')
    assert component_name_label.properties()['text'] == 'Capacitor Bipolar'
    package_name_label = library_editor.widget('libraryEditorNewElementWizardDevicePropertiesPackageNameLabel')
    assert package_name_label.properties()['text'] == 'C-0805'

    # Finish
    dialog = library_editor.widget('libraryEditorNewElementWizard')
    library_editor.widget('libraryEditorNewElementWizardFinishButton').click()
    helpers.wait_until_widget_hidden(dialog)
