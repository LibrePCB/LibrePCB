#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the device editor
"""

from slint_testing import keys


def test_edit_categories(librepcb):
    """
    Test adding and removing categories with the category chooser dialog.

    Note: The full coverage of the category chooser dialog (expanding/
    collapsing, filtering, keyboard shortcuts, ...) is done in the symbol
    editor test (test_symbol_editor.py). This test only covers the basic
    usage of the dialog in the device editor to reduce redundancy.
    """
    librepcb.add_local_library_to_workspace("libraries/Populated Library.lplib")
    with librepcb.open() as app:
        # Open the existing C-0805 device and switch to the metadata page.
        app.get("SideBar::libraries-btn").click()
        app.get("LibrariesPanel::local-libs #LibraryListViewItem").dclick()
        items = app.get("LibraryContentTab::elements-view LibraryTreeView::item-ta *")
        items.wait_for_item(lambda item: item.label == "C-0805").dclick()
        nav_items = app.get("DeviceTab::navbar NavBar::item-btn *")
        nav_items.wait_for_item(lambda item: item.label == "Metadata").click()
        dialog = app.get("DeviceTab::add-category-dialog ?")
        tree_items = dialog.get("TreeView::item-ta *")

        # Helpers
        def _open_dialog():
            app.get(
                "DeviceMetadataTab::categories-cell CategoriesRowCell::add-ta"
            ).click()
            dialog.wait(1)

        def _remove_category(category):
            # The remove button is only visible while the pointer hovers
            # over the corresponding row, so move the pointer there first.
            item = app.get(
                "DeviceMetadataTab::categories-cell CategoriesRowCell::item-ta *"
            ).wait_for_item(lambda item: item.label == category)
            app.hover(item.rect.center_abs)
            # Only the hovered row's remove button is visible.
            app.get(
                "DeviceMetadataTab::categories-cell CategoriesRowCell::item-remove-btn"
            ).click()

        def _verify_result(categories):
            dialog.wait_for(valid=False)
            app.get(
                "DeviceMetadataTab::categories-cell CategoriesRowCell::item-ta *"
            ).wait_for(label=categories)

        # Test 1:
        # Type a filter term, then pick the "LEDs" category with the mouse.
        # This closes the dialog and adds the category to the device.
        # Note: The device already has the "Capacitors" category, which must
        # be kept in addition to the newly added "LEDs" category.
        _open_dialog()
        dialog.get("TreeViewDialog::filter-edt").set_value("led")
        tree_items.wait_for(label=["Optoelectronics", "LEDs"])
        tree_items.wait_for_item(lambda item: item.label == "LEDs").click()
        _verify_result(["Capacitors", "LEDs"])

        # Test 2:
        # Remove the "Capacitors" category with the mouse. The previously
        # added "LEDs" category must be kept.
        _remove_category("Capacitors")
        _verify_result(["LEDs"])

        # Test 3:
        # Open the dialog again and close it with the Escape key. No category
        # must be added when the dialog is closed this way.
        _open_dialog()
        app.trigger_shortcut([keys.Escape])
        _verify_result(["LEDs"])


def test_open_package_and_component(librepcb):
    librepcb.add_local_library_to_workspace("libraries/Populated Library.lplib")
    with librepcb.open() as app:
        app.get("SideBar::libraries-btn").click()
        app.get("LibrariesPanel::local-libs #LibraryListViewItem").dclick()
        tabs = app.get("#TabButton *")

        # Open device
        items = app.get("LibraryContentTab::elements-view LibraryTreeView::item-ta *")
        items.wait_for_item(lambda item: item.label == "C-0805").dclick()
        tabs.wait(3)
        assert tabs[-1].label == "C-0805"

        # Open package
        app.get(
            "DeviceContentTab::package-card DeviceDependencyCard::title-btn"
        ).click()
        tabs.wait(4)
        assert tabs[-1].label == "C-0805"
        tabs[-1].mclick()

        # Open component
        app.get(
            "DeviceContentTab::component-card-{1,2} DeviceDependencyCard::title-btn"
        ).click()
        tabs.wait(4)
        assert tabs[-1].label == "Capacitor Bipolar"
        tabs[-1].mclick()


# Regression test for a crash in LibrePCB 2.1.0.
def test_part_attributes_editor_cancel(librepcb):
    librepcb.add_local_library_to_workspace("libraries/Populated Library.lplib")
    with librepcb.open() as app:
        app.get("SideBar::libraries-btn").click()
        app.get("LibrariesPanel::local-libs #LibraryListViewItem").dclick()
        tabs = app.get("#TabButton *")

        # Open device
        items = app.get("LibraryContentTab::elements-view LibraryTreeView::item-ta *")
        items.wait_for_item(lambda item: item.label == "C-0805").dclick()
        tabs.wait(3)
        assert tabs[-1].label == "C-0805"

        # Open attributes editor of new part and focus the key edit
        app.get("PartListView::attributes-ta").click()
        overlay = app.get("DeviceContentTab::attributes-overlay")
        overlay.get("AttributeListView::key-edt").click()

        # Close the attributes editor with the Escape key and verify that this
        # closes the overlay. This operation caused a crash in LibrePCB 2.1.0.
        app.trigger_shortcut([keys.Escape])
        overlay.wait_for(valid=False)
