#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test creating a component with the library editor
"""


def test(library_editor, helpers):
    """
    Create new component
    """
    le = library_editor

    # Open "New Library Element" wizard
    le.action('libraryEditorActionNewElement').trigger(blocking=False)

    # Choose type of element
    le.widget('libraryEditorNewElementWizardChooseTypeComponentButton').click()

    # Enter metadata
    widget_properties = {
        ('NameEdit', 'text'): 'New Component',
        ('DescriptionEdit', 'plainText'): 'Foo Bar',
        ('KeywordsEdit', 'text'): '',
        ('AuthorEdit', 'text'): 'Functional Test',
        ('VersionEdit', 'text'): '1.2.3',
    }
    for (widget, property), value in widget_properties.items():
        le.widget('libraryEditorNewElementWizardMetadata' + widget).set_property(property, value)
    le.widget('libraryEditorNewElementWizardNextButton').click()

    # Enter component properties
    widget_properties = {
        ('SchematicOnlyComboBox', 'checked'): False,
        ('DefaultValueEdit', 'plainText'): 'Default Value',
        ('PrefixEdit', 'text'): 'X',
    }
    for (widget, property), value in widget_properties.items():
        le.widget('libraryEditorNewElementWizardComponentProperties' + widget).set_property(property, value)
    le.widget('libraryEditorNewElementWizardNextButton').click()

    # Add a symbol
    le.widget('libraryEditorNewElementWizardComponentSymbolsChooseSymbolButton').click()
    category_tree = le.widget('libraryEditorNewElementWizardComponentSymbolsChooseSymbolDialogCategoriesTree')
    helpers.wait_for_model_items_count(category_tree, 1)
    category = category_tree.model().items().items[0]
    category_tree.select_item(category)
    symbols_list = le.widget('libraryEditorNewElementWizardComponentSymbolsChooseSymbolDialogSymbolsList')
    helpers.wait_for_model_items_count(symbols_list, 1)
    symbol = symbols_list.model().items().items[0]
    symbols_list.select_item(symbol)
    le.widget('libraryEditorNewElementWizardComponentSymbolsChooseSymbolDialogAcceptButton').click()
    le.widget('libraryEditorNewElementWizardComponentSymbolsAddSymbolButton').click()
    le.widget('libraryEditorNewElementWizardNextButton').click()

    # Define signals
    # (Do nothing as signals are automatically generated)
    le.widget('libraryEditorNewElementWizardNextButton').click()

    # Define pin-signal-map
    # (Do nothing as pin-signal-map is automatically generated)

    # Finish
    dialog = le.widget('libraryEditorNewElementWizard')
    le.widget('libraryEditorNewElementWizardFinishButton').click()
    helpers.wait_until_widget_hidden(dialog)

    # Check if a new tab is opened (indicates that the element was created)
    tab_props = le.widget('libraryEditorStackedWidget').properties()
    assert tab_props['count'] == 2
    assert tab_props['currentIndex'] == 1

    # Check metadata
    assert le.widget('libraryEditorComponentNameEdit').properties()['text'] == 'New Component'
    assert le.widget('libraryEditorComponentDescriptionEdit').properties()['plainText'] == 'Foo Bar'
    assert le.widget('libraryEditorComponentKeywordsEdit').properties()['text'] == ''
    assert le.widget('libraryEditorComponentAuthorEdit').properties()['text'] == 'Functional Test'
    assert le.widget('libraryEditorComponentVersionEdit').properties()['text'] == '1.2.3'
