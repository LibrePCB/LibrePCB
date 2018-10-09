#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test creating a device with the library editor
"""

import time


def test(library_editor, helpers):
    """
    Create new device
    """

    # Open "New Library Element" wizard
    library_editor.action('libraryEditorActionNewElement').trigger(blocking=False)

    # Choose type of element
    library_editor.widget('libraryEditorNewElementWizardChooseTypeDeviceButton').click()

    # Enter metadata
    widget_properties = {
        ('NameEdit', 'text'): 'New Device',
        ('DescriptionEdit', 'text'): 'Foo Bar',
        ('KeywordsEdit', 'text'): '',
        ('AuthorEdit', 'text'): 'Functional Test',
        ('VersionEdit', 'text'): '1.2.3',
    }
    for (widget, property), value in widget_properties.items():
        library_editor.widget('libraryEditorNewElementWizardMetadata' + widget).set_property(property, value)
    time.sleep(0.5)  # Workaround for https://github.com/parkouss/funq/issues/39
    library_editor.widget('libraryEditorNewElementWizardNextButton').click()

    # Select component
    library_editor.widget('libraryEditorNewElementWizardDevicePropertiesChooseComponentButton').click()
    category_tree = library_editor.widget('libraryEditorNewElementWizardDevicePropertiesChooseComponentDialogCategoriesTree')
    category_tree.model_items().items[0].select()
    components_list = library_editor.widget('libraryEditorNewElementWizardDevicePropertiesChooseComponentDialogComponentsList')
    components_list.model_items().items[0].select()
    library_editor.widget('libraryEditorNewElementWizardDevicePropertiesChooseComponentDialogAcceptButton').click()

    # Select package
    library_editor.widget('libraryEditorNewElementWizardDevicePropertiesChoosePackageButton').click()
    category_tree = library_editor.widget('libraryEditorNewElementWizardDevicePropertiesChoosePackageDialogCategoriesTree')
    category_tree.model_items().items[2].select()
    packages_list = library_editor.widget('libraryEditorNewElementWizardDevicePropertiesChoosePackageDialogPackagesList')
    packages_list.model_items().items[0].select()
    library_editor.widget('libraryEditorNewElementWizardDevicePropertiesChoosePackageDialogAcceptButton').click()

    # Finish
    dialog = library_editor.widget('libraryEditorNewElementWizard')
    library_editor.widget('libraryEditorNewElementWizardFinishButton').click()
    helpers.wait_until_widget_hidden(dialog)
