#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the symbol editor
"""

from slint_testing import keys


def test_edit_categories(librepcb):
    """
    Test adding categories with the category chooser dialog, and removing them.
    """
    librepcb.add_local_library_to_workspace("libraries/Populated Library.lplib")

    with librepcb.open() as app:
        # Start creating a new symbol with the wizard.
        app.get("SideBar::libraries-btn").click()
        app.get("LibrariesPanel::local-libs #LibraryListViewItem").dclick()
        app.get("LibraryContentTab::new-sym-btn").click()
        dialog = app.get("SymbolMetadataTab::add-category-dialog ?")
        tree_items = dialog.get("TreeView::item-ta *")

        # Helpers
        def _open_dialog():
            app.get(
                "SymbolMetadataTab::categories-cell CategoriesRowCell::add-ta"
            ).click()
            dialog.wait(1)

        def _remove_category(category):
            # The remove button is only visible while the pointer hovers
            # over the corresponding row, so move the pointer there first.
            item = app.get(
                "SymbolMetadataTab::categories-cell CategoriesRowCell::item-ta *"
            ).wait_for_item(lambda item: item.label == category)
            app.hover(item.rect.center_abs)
            # Only the hovered row's remove button is visible.
            app.get(
                "SymbolMetadataTab::categories-cell CategoriesRowCell::item-remove-btn"
            ).click()

        def _verify_result(categories):
            dialog.wait_for(valid=False)
            app.get(
                "SymbolMetadataTab::categories-cell CategoriesRowCell::item-ta *"
            ).wait_for(label=categories)

        # Test 1:
        # Expand the "Optoelectronics" category to make its child category
        # "LEDs" visible. Then collapse it again and expand it once more to
        # verify that toggling the expansion state works in both directions.
        # In the end, click on the "Optoelectronics" category to pick it,
        # which closes the dialog and adds the category to the symbol.
        _open_dialog()
        tree_items.wait_for_item(lambda item: item.label == "Optoelectronics").expand()
        tree_items.wait_for_item(lambda item: item.label == "LEDs")
        tree_items.wait_for_item(lambda item: item.label == "Optoelectronics").expand()
        tree_items.wait_for_no_item(lambda item: item.label == "LEDs")
        tree_items.wait_for_item(lambda item: item.label == "Optoelectronics").expand()
        tree_items.wait_for_item(lambda item: item.label == "LEDs")
        tree_items.wait_for_item(lambda item: item.label == "Optoelectronics").click()
        _verify_result(["Optoelectronics"])

        # Test 2:
        # Close the dialog by clicking the close button. The expansion state of
        # the previous dialog is still applied.
        _open_dialog()
        tree_items.wait_for_item(lambda item: item.label == "LEDs")
        dialog.get("OverlayDialog::close-btn").click()
        _verify_result(["Optoelectronics"])  # No change

        # Test 3:
        # Type "led" into the filter edit, then pick the "LEDs" category with
        # the keyboard.
        _open_dialog()
        dialog.get("TreeViewDialog::filter-edt").set_value("led")
        tree_items.wait_for(label=["Optoelectronics", "LEDs"])
        app.trigger_shortcut([keys.DownArrow])  # Select "Optoelectronics"
        app.trigger_shortcut([keys.DownArrow])  # Select "LEDs"
        app.trigger_shortcut([keys.Return])
        _verify_result(["Optoelectronics", "LEDs"])

        # Test 4:
        # Pick a category with the return key directly (without selecting it
        # first), using a filter term which matches exactly one category. The
        # term "resistance" matches only the keyword (not the name) of the
        # "Resistors" category, so we also test the keywords filtering here.
        _open_dialog()
        dialog.get("TreeViewDialog::filter-edt").set_value("resistance")
        tree_items.wait_for(label=["Resistors"])
        app.trigger_shortcut([keys.Return])
        _verify_result(["Optoelectronics", "LEDs", "Resistors"])

        # Test 5:
        # The escape key shall clear the filter if any, or close the dialog if
        # the filter is already empty. We also verify that a filter which
        # doesn't match any category leads to an empty list.
        _open_dialog()
        dialog.get("TreeViewDialog::filter-edt").set_value("foobar")
        tree_items.wait(0)
        app.trigger_shortcut([keys.Escape])  # Clears the filter
        dialog.get("TreeViewDialog::filter-edt").wait_for(value="")
        tree_items.wait_for_item(lambda item: item.label == "LEDs")
        app.trigger_shortcut([keys.Escape])  # Closes the dialog
        _verify_result(["Optoelectronics", "LEDs", "Resistors"])

        # Test 6:
        # Remove all categories one-by-one with the mouse.
        _remove_category("LEDs")
        _verify_result(["Optoelectronics", "Resistors"])
        _remove_category("Resistors")
        _verify_result(["Optoelectronics"])
        _remove_category("Optoelectronics")
        _verify_result([])
