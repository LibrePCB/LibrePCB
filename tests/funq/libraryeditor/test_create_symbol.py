#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test creating a symbol with the library editor
"""


def test(library_editor, helpers):
    """
    Create new symbol
    """
    le = library_editor

    # Open "New Library Element" wizard
    le.action('libraryEditorActionNewElement').trigger(blocking=False)

    # Choose type of element
    le.widget('libraryEditorNewElementWizardChooseTypeSymbolButton').click()

    # Enter metadata
    widget_properties = {
        ('NameEdit', 'text'): 'New Symbol',
        ('DescriptionEdit', 'plainText'): 'Foo Bar',
        ('KeywordsEdit', 'text'): '',
        ('AuthorEdit', 'text'): 'Functional Test',
        ('VersionEdit', 'text'): '1.2.3',
    }
    for (widget, property), value in widget_properties.items():
        le.widget('libraryEditorNewElementWizardMetadata' + widget).set_property(property, value)

    # Finish
    dialog = le.widget('libraryEditorNewElementWizard')
    le.widget('libraryEditorNewElementWizardFinishButton').click()
    helpers.wait_until_widget_hidden(dialog)

    # Check if a new tab is opened (indicates that the element was created)
    tab_props = le.widget('libraryEditorStackedWidget').properties()
    assert tab_props['count'] == 2
    assert tab_props['currentIndex'] == 1

    # Check metadata
    assert le.widget('libraryEditorSymbolNameEdit').properties()['text'] == 'New Symbol'
    assert le.widget('libraryEditorSymbolDescriptionEdit').properties()['plainText'] == 'Foo Bar'
    assert le.widget('libraryEditorSymbolKeywordsEdit').properties()['text'] == ''
    assert le.widget('libraryEditorSymbolAuthorEdit').properties()['text'] == 'Functional Test'
    assert le.widget('libraryEditorSymbolVersionEdit').properties()['text'] == '1.2.3'
