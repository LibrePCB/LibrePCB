#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test creating a component with the library editor
"""

import time


def test(library_editor, helpers):
    """
    Create new component
    """

    # Open "New Library Element" wizard
    library_editor.action('libraryEditorActionNewElement').trigger(blocking=False)

    # Choose type of element
    library_editor.widget('libraryEditorNewElementWizardChooseTypeComponentButton').click()

    # Enter metadata
    widget_properties = {
        ('NameEdit', 'text'): 'New Component',
        ('DescriptionEdit', 'text'): 'Foo Bar',
        ('KeywordsEdit', 'text'): '',
        ('AuthorEdit', 'text'): 'Functional Test',
        ('VersionEdit', 'text'): '1.2.3',
    }
    for (widget, property), value in widget_properties.items():
        library_editor.widget('libraryEditorNewElementWizardMetadata' + widget).set_property(property, value)
    time.sleep(0.5)  # Workaround for https://github.com/parkouss/funq/issues/39
    library_editor.widget('libraryEditorNewElementWizardNextButton').click()

    # Enter component properties
    widget_properties = {
        ('SchematicOnlyComboBox', 'checked'): False,
        ('DefaultValueEdit', 'plainText'): 'Default Value',
        ('PrefixEdit', 'text'): 'X',
    }
    for (widget, property), value in widget_properties.items():
        library_editor.widget('libraryEditorNewElementWizardComponentProperties' + widget).set_property(property, value)
    time.sleep(0.5)  # Workaround for https://github.com/parkouss/funq/issues/39
    library_editor.widget('libraryEditorNewElementWizardNextButton').click()

    # Add a symbol
    library_editor.widget('libraryEditorNewElementWizardComponentSymbolsChooseSymbolButton').click()
    category_tree = library_editor.widget('libraryEditorNewElementWizardComponentSymbolsChooseSymbolDialogCategoriesTree')
    category_tree.model_items().items[0].select()
    symbols_list = library_editor.widget('libraryEditorNewElementWizardComponentSymbolsChooseSymbolDialogSymbolsList')
    symbols_list.model_items().items[0].select()
    library_editor.widget('libraryEditorNewElementWizardComponentSymbolsChooseSymbolDialogAcceptButton').click()
    library_editor.widget('libraryEditorNewElementWizardComponentSymbolsAddSymbolButton').click()
    library_editor.widget('libraryEditorNewElementWizardNextButton').click()

    # Define signals
    # (Do nothing as signals are automatically generated)
    library_editor.widget('libraryEditorNewElementWizardNextButton').click()

    # Define pin-signal-map
    # (Do nothing as pin-signal-map is automatically generated)

    # Finish
    dialog = library_editor.widget('libraryEditorNewElementWizard')
    library_editor.widget('libraryEditorNewElementWizardFinishButton').click()
    helpers.wait_until_widget_hidden(dialog)
