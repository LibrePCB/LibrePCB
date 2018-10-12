#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test copying a symbol in the library editor
"""


def test(library_editor, helpers):
    """
    Copy symbol with "New Library Element" wizard
    """

    # Open "New Library Element" wizard
    library_editor.action('libraryEditorActionNewElement').trigger(blocking=False)

    # Choose "Copy existing element"
    library_editor.widget('libraryEditorNewElementWizardChooseTypeCopyRadioButton').set_property('checked', True)

    # Choose type of element
    library_editor.widget('libraryEditorNewElementWizardChooseTypeSymbolButton').click()

    # Choose category
    category_tree = library_editor.widget('libraryEditorNewElementWizardCopyFromCategoriesTree')
    category_tree.model_items().items[0].select()

    # Choose element
    element_list = library_editor.widget('libraryEditorNewElementWizardCopyFromElementsList')
    element_list.model_items().items[0].select()
    library_editor.widget('libraryEditorNewElementWizardNextButton').click()

    # Check metadata
    widget_properties = {
        ('NameEdit', 'text'): 'Capacitor Bipolar EU',
        ('DescriptionEdit', 'plainText'): 'Bipolar Capacitor European (IEC 60617)',
        ('KeywordsEdit', 'text'): 'capacitor,capacitance,bipolar',
        ('VersionEdit', 'text'): '0.1',
    }
    for (widget, property), value in widget_properties.items():
        props = library_editor.widget('libraryEditorNewElementWizardMetadata' + widget).properties()
        assert props[property] == value

    # Finish
    dialog = library_editor.widget('libraryEditorNewElementWizard')
    library_editor.widget('libraryEditorNewElementWizardFinishButton').click()
    helpers.wait_until_widget_hidden(dialog)
