#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test creating a package category with the library editor
"""

import time


def test(library_editor, helpers):
    """
    Create new package category
    """

    # Open "New Library Element" wizard
    library_editor.action('libraryEditorActionNewElement').trigger(blocking=False)

    # Choose type of element
    library_editor.widget('libraryEditorNewElementWizardChooseTypePkgCatButton').click()

    # Enter metadata
    widget_properties = {
        ('NameEdit', 'text'): 'New Package Category',
        ('DescriptionEdit', 'text'): 'Foo Bar',
        ('KeywordsEdit', 'text'): '',
        ('AuthorEdit', 'text'): 'Functional Test',
        ('VersionEdit', 'text'): '1.2.3',
    }
    for (widget, property), value in widget_properties.items():
        library_editor.widget('libraryEditorNewElementWizardMetadata' + widget).set_property(property, value)
    time.sleep(0.5)  # Workaround for https://github.com/parkouss/funq/issues/39

    # Finish
    dialog = library_editor.widget('libraryEditorNewElementWizard')
    library_editor.widget('libraryEditorNewElementWizardFinishButton').click()
    helpers.wait_until_widget_hidden(dialog)
