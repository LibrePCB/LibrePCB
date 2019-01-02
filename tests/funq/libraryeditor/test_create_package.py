#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test creating a package with the library editor
"""


def test(library_editor, helpers):
    """
    Create new package
    """

    # Open "New Library Element" wizard
    library_editor.action('libraryEditorActionNewElement').trigger(blocking=False)

    # Choose type of element
    library_editor.widget('libraryEditorNewElementWizardChooseTypePackageButton').click()

    # Enter metadata
    widget_properties = {
        ('NameEdit', 'text'): 'New Package',
        ('DescriptionEdit', 'text'): 'Foo Bar',
        ('KeywordsEdit', 'text'): '',
        ('AuthorEdit', 'text'): 'Functional Test',
        ('VersionEdit', 'text'): '1.2.3',
    }
    for (widget, property), value in widget_properties.items():
        library_editor.widget('libraryEditorNewElementWizardMetadata' + widget).set_property(property, value)
    library_editor.widget('libraryEditorNewElementWizardNextButton').click()

    # Finish
    dialog = library_editor.widget('libraryEditorNewElementWizard')
    library_editor.widget('libraryEditorNewElementWizardFinishButton').click()
    helpers.wait_until_widget_hidden(dialog)
