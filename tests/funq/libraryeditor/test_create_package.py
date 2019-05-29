#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test creating a package with the library editor
"""


def test(library_editor, helpers):
    """
    Create new package
    """
    le = library_editor

    # Open "New Library Element" wizard
    le.action('libraryEditorActionNewElement').trigger(blocking=False)

    # Choose type of element
    le.widget('libraryEditorNewElementWizardChooseTypePackageButton').click()

    # Enter metadata
    widget_properties = {
        ('NameEdit', 'text'): 'New Package',
        ('DescriptionEdit', 'plainText'): 'Foo Bar',
        ('KeywordsEdit', 'text'): '',
        ('AuthorEdit', 'text'): 'Functional Test',
        ('VersionEdit', 'text'): '1.2.3',
    }
    for (widget, property), value in widget_properties.items():
        le.widget('libraryEditorNewElementWizardMetadata' + widget).set_property(property, value)
    le.widget('libraryEditorNewElementWizardNextButton').click()

    # Finish
    dialog = le.widget('libraryEditorNewElementWizard')
    le.widget('libraryEditorNewElementWizardFinishButton').click()
    helpers.wait_until_widget_hidden(dialog)

    # Check if a new tab is opened (indicates that the element was created)
    tab_props = le.widget('libraryEditorStackedWidget').properties()
    assert tab_props['count'] == 2
    assert tab_props['currentIndex'] == 1

    # Check metadata
    assert le.widget('libraryEditorPackageNameEdit').properties()['text'] == 'New Package'
    assert le.widget('libraryEditorPackageDescriptionEdit').properties()['plainText'] == 'Foo Bar'
    assert le.widget('libraryEditorPackageKeywordsEdit').properties()['text'] == ''
    assert le.widget('libraryEditorPackageAuthorEdit').properties()['text'] == 'Functional Test'
    assert le.widget('libraryEditorPackageVersionEdit').properties()['text'] == '1.2.3'
