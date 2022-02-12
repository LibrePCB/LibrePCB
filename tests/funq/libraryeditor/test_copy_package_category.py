#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test copying a package category in the library editor
"""


def test(library_editor, helpers):
    """
    Copy package category with "New Library Element" wizard
    """
    le = library_editor

    # Open "New Library Element" wizard
    le.action('libraryEditorActionNewElement').trigger(blocking=False)

    # Choose "Copy existing element"
    le.widget('libraryEditorNewElementWizardChooseTypeCopyRadioButton').set_property('checked', True)

    # Choose type of element
    le.widget('libraryEditorNewElementWizardChooseTypePkgCatButton').click()

    # Choose category
    category_tree = le.widget('libraryEditorNewElementWizardCopyFromCategoriesTree')
    helpers.wait_for_model_items_count(category_tree, 4)
    category = category_tree.model().items().items[2]
    category_tree.select_item(category)
    le.widget('libraryEditorNewElementWizardNextButton').click()

    # Check metadata
    widget_properties = {
        # Note: C-SMT/Crystals are buggy due to Qt/ICU bug in sort algorithm!
        ('NameEdit', 'text'): 'R-SMT',
        ('DescriptionEdit', 'plainText'): '',
        ('KeywordsEdit', 'text'): '',
        ('VersionEdit', 'text'): '0.1',
    }
    for (widget, property), value in widget_properties.items():
        props = le.widget('libraryEditorNewElementWizardMetadata' + widget).properties()
        assert props[property] == value

    # Finish
    dialog = le.widget('libraryEditorNewElementWizard')
    le.widget('libraryEditorNewElementWizardFinishButton').click()
    helpers.wait_until_widget_hidden(dialog)

    # Check if a new tab is opened (indicates that the element was created)
    tab_props = le.widget('libraryEditorStackedWidget').properties()
    assert tab_props['count'] == 2
    assert tab_props['currentIndex'] == 1

    # Check metadata
    assert le.widget('libraryEditorPkgCatNameEdit').properties()['text'] == 'R-SMT'
    assert le.widget('libraryEditorPkgCatDescriptionEdit').properties()['plainText'] == ''
    assert le.widget('libraryEditorPkgCatKeywordsEdit').properties()['text'] == ''
    assert le.widget('libraryEditorPkgCatVersionEdit').properties()['text'] == '0.1'
