#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test copying a component in the library editor
"""


def test(library_editor, helpers):
    """
    Copy component with "New Library Element" wizard
    """

    # Open "New Library Element" wizard
    library_editor.action('libraryEditorActionNewElement').trigger(blocking=False)

    # Choose "Copy existing element"
    library_editor.widget('libraryEditorNewElementWizardChooseTypeCopyRadioButton').set_property('checked', True)

    # Choose type of element
    library_editor.widget('libraryEditorNewElementWizardChooseTypeComponentButton').click()

    # Choose category
    category_tree = library_editor.widget('libraryEditorNewElementWizardCopyFromCategoriesTree')
    helpers.wait_for_model_items_count(category_tree, 1)
    category = category_tree.model().items().items[0]
    category_tree.select_item(category)

    # Choose element
    element_list = library_editor.widget('libraryEditorNewElementWizardCopyFromElementsList')
    helpers.wait_for_model_items_count(element_list, 1)
    element = element_list.model().items().items[0]
    element_list.select_item(element)
    library_editor.widget('libraryEditorNewElementWizardNextButton').click()

    # Check metadata
    widget_properties = {
        ('NameEdit', 'text'): 'Capacitor Bipolar',
        ('DescriptionEdit', 'plainText'): '',
        ('KeywordsEdit', 'text'): 'capacitor,capacitance,bipolar',
        ('VersionEdit', 'text'): '0.1',
    }
    for (widget, property), value in widget_properties.items():
        props = library_editor.widget('libraryEditorNewElementWizardMetadata' + widget).properties()
        assert props[property] == value
    library_editor.widget('libraryEditorNewElementWizardNextButton').click()

    # Check component properties
    widget_properties = {
        ('SchematicOnlyComboBox', 'checked'): False,
        ('DefaultValueEdit', 'plainText'): '{{CAPACITANCE}}',
        ('PrefixEdit', 'text'): 'C',
    }
    for (widget, property), value in widget_properties.items():
        props = library_editor.widget('libraryEditorNewElementWizardComponentProperties' + widget).properties()
        assert props[property] == value
    library_editor.widget('libraryEditorNewElementWizardNextButton').click()

    # Skip choosing symbols
    library_editor.widget('libraryEditorNewElementWizardNextButton').click()

    # Skip choosing signals
    library_editor.widget('libraryEditorNewElementWizardNextButton').click()

    # Finish
    dialog = library_editor.widget('libraryEditorNewElementWizard')
    library_editor.widget('libraryEditorNewElementWizardFinishButton').click()
    helpers.wait_until_widget_hidden(dialog)
