#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pytest

"""
Test "add component"-dialog in project editor
"""


class AddComponentDialogHelper(object):
    def __init__(self, app):
        super(AddComponentDialogHelper, self).__init__()
        self.app = app
        self.app.action('schematicEditorActionAddComponent').trigger(blocking=False)
        self.dialog = self.app.widget('schematicEditorAddComponentDialog')


@pytest.fixture
def add_component_dialog(project_editor, helpers):
    """
    Fixture opening the "Add Component"-editor in the schematic editor
    """
    helpers.wait_for_library_scan_complete(project_editor)
    helper = AddComponentDialogHelper(project_editor)
    yield helper


def test_if_dialog_is_active_widget(add_component_dialog, helpers):
    """
    Test if the dialog is the active widget after opening it
    """
    helpers.wait_for_active_dialog(add_component_dialog.app, add_component_dialog.dialog)  # raises on timeout


def test_init_state(add_component_dialog, helpers):
    """
    Test if the initial state of various widgets is correct
    """
    # Category tree must be populated
    category_tree = add_component_dialog.app.widget('schematicEditorAddComponentDialogCategoryTree')
    helpers.wait_for_model_items_count(category_tree, 1)
    assert category_tree.model_items().items[0].value == 'Capacitors'

    # Component tree must be empty
    component_tree = add_component_dialog.app.widget('schematicEditorAddComponentDialogComponentTree')
    helpers.wait_for_model_items_count(component_tree, 0, 0)

    # Symbol variant combobox must be hidden
    symbvar_combobox = add_component_dialog.app.widget('schematicEditorAddComponentDialogSymVarCombobox', wait_active=False)
    assert symbvar_combobox.properties()['visible'] is False


def test_if_cancel_closes_dialog(add_component_dialog, helpers):
    """
    Test if the cancel button closes the dialog
    """
    add_component_dialog.app.widget('schematicEditorAddComponentDialogButtonCancel').click()
    helpers.wait_until_widget_hidden(add_component_dialog.dialog)  # raises on timeout


def test_if_accept_does_nothing_if_no_component_selected(add_component_dialog):
    """
    Test if the accept button does nothing if no component is selected
    """
    add_component_dialog.app.widget('schematicEditorAddComponentDialogButtonAccept').click()
    assert add_component_dialog.dialog.properties()['visible'] is True


def test_select_component_with_one_symbvar(add_component_dialog, helpers):
    """
    Test selecting a component with only one symbol variant
    """
    # Select category
    category_tree = add_component_dialog.app.widget('schematicEditorAddComponentDialogCategoryTree')
    helpers.wait_for_model_items_count(category_tree, 3)
    category = category_tree.model_items().items[2]
    assert category.value == 'Diodes'
    category.select()

    # Select component
    component_tree = add_component_dialog.app.widget('schematicEditorAddComponentDialogComponentTree')
    helpers.wait_for_model_items_count(component_tree, 1)
    component = component_tree.model_items().items[0]
    assert component.value == 'Diode'
    component.select()

    # Symbol variant must be hidden
    symbvar_combobox = add_component_dialog.app.widget('schematicEditorAddComponentDialogSymVarCombobox', wait_active=False)
    assert symbvar_combobox.properties()['visible'] is False

    # Check labels
    component_name_label = add_component_dialog.app.widget('schematicEditorAddComponentDialogComponentNameLabel')
    assert component_name_label.properties()['text'] == 'Diode'
    device_name_label = add_component_dialog.app.widget('schematicEditorAddComponentDialogDeviceNameLabel')
    assert device_name_label.properties()['text'] == 'No device selected'

    # Accept
    add_component_dialog.app.widget('schematicEditorAddComponentDialogButtonAccept').click()
    helpers.wait_until_widget_hidden(add_component_dialog.dialog)  # raises on timeout


def test_select_component_with_two_symbvars(add_component_dialog, helpers):
    """
    Test selecting a component with two symbol variants
    """
    # Select category
    category_tree = add_component_dialog.app.widget('schematicEditorAddComponentDialogCategoryTree')
    helpers.wait_for_model_items_count(category_tree, 1)
    category = category_tree.model_items().items[0]
    assert category.value == 'Capacitors'
    category.select()

    # Select component
    component_tree = add_component_dialog.app.widget('schematicEditorAddComponentDialogComponentTree')
    helpers.wait_for_model_items_count(component_tree, 1)
    component = component_tree.model_items().items[0]
    assert component.value == 'Capacitor Bipolar'
    component.select()

    # Symbol variant must be visible
    symbvar_combobox = add_component_dialog.app.widget('schematicEditorAddComponentDialogSymVarCombobox')
    helpers.wait_for_model_items_count(symbvar_combobox, 2, 2)
    symbvar_items = symbvar_combobox.model_items()
    assert symbvar_items.items[0].value == 'European [IEC 60617]'
    assert symbvar_items.items[1].value == 'American [IEEE 315]'
    assert symbvar_combobox.properties()['currentText'] == symbvar_items.items[0].value

    # Check labels
    component_name_label = add_component_dialog.app.widget('schematicEditorAddComponentDialogComponentNameLabel')
    assert component_name_label.properties()['text'] == 'Capacitor Bipolar'
    device_name_label = add_component_dialog.app.widget('schematicEditorAddComponentDialogDeviceNameLabel')
    assert device_name_label.properties()['text'] == 'No device selected'

    # Accept
    add_component_dialog.app.widget('schematicEditorAddComponentDialogButtonAccept').click()
    helpers.wait_until_widget_hidden(add_component_dialog.dialog)  # raises on timeout


def test_select_device(add_component_dialog, helpers):
    """
    Test selecting a device
    """
    # Select category
    category_tree = add_component_dialog.app.widget('schematicEditorAddComponentDialogCategoryTree')
    helpers.wait_for_model_items_count(category_tree, 1)
    category = category_tree.model_items().items[0]
    assert category.value == 'Capacitors'
    category.select()

    # Select device
    component_tree = add_component_dialog.app.widget('schematicEditorAddComponentDialogComponentTree')
    helpers.wait_for_model_items_count(component_tree, 1)
    device = component_tree.model_items().items[0].items[1]
    assert device.value == 'C-0805'
    device.select()

    # Symbol variant must be visible
    symbvar_combobox = add_component_dialog.app.widget('schematicEditorAddComponentDialogSymVarCombobox')
    helpers.wait_for_model_items_count(symbvar_combobox, 2, 2)
    symbvar_items = symbvar_combobox.model_items()
    assert symbvar_items.items[0].value == 'European [IEC 60617]'
    assert symbvar_items.items[1].value == 'American [IEEE 315]'
    assert symbvar_combobox.properties()['currentText'] == symbvar_items.items[0].value

    # Check labels
    component_name_label = add_component_dialog.app.widget('schematicEditorAddComponentDialogComponentNameLabel')
    assert component_name_label.properties()['text'] == 'Capacitor Bipolar'
    device_name_label = add_component_dialog.app.widget('schematicEditorAddComponentDialogDeviceNameLabel')
    assert device_name_label.properties()['text'] == 'C-0805'

    # Accept
    add_component_dialog.app.widget('schematicEditorAddComponentDialogButtonAccept').click()
    helpers.wait_until_widget_hidden(add_component_dialog.dialog)  # raises on timeout


def test_select_component_by_doubleclick(add_component_dialog, helpers):
    """
    Test selecting a component by double-clicking the tree item
    """
    # Select category
    category_tree = add_component_dialog.app.widget('schematicEditorAddComponentDialogCategoryTree')
    helpers.wait_for_model_items_count(category_tree, 3)
    category = category_tree.model_items().items[2]
    assert category.value == 'Diodes'
    category.select()

    # Select component
    component_tree = add_component_dialog.app.widget('schematicEditorAddComponentDialogComponentTree')
    helpers.wait_for_model_items_count(component_tree, 1)
    component = component_tree.model_items().items[0]
    assert component.value == 'Diode'
    component.dclick()

    # Check if dialog is closed
    helpers.wait_until_widget_hidden(add_component_dialog.dialog)  # raises on timeout


def test_select_device_by_doubleclick(add_component_dialog, helpers):
    """
    Test selecting a device by double-clicking the tree item
    """
    # Select category
    category_tree = add_component_dialog.app.widget('schematicEditorAddComponentDialogCategoryTree')
    helpers.wait_for_model_items_count(category_tree, 1)
    category = category_tree.model_items().items[0]
    assert category.value == 'Capacitors'
    category.select()

    # Select device
    component_tree = add_component_dialog.app.widget('schematicEditorAddComponentDialogComponentTree')
    helpers.wait_for_model_items_count(component_tree, 1)
    device = component_tree.model_items().items[0].items[1]
    assert device.value == 'C-0805'
    device.dclick()

    # Check if dialog is closed
    helpers.wait_until_widget_hidden(add_component_dialog.dialog)  # raises on timeout
