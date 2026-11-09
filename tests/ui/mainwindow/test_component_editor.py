#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the component editor
"""

from slint_testing import keys


def test_edit_categories(librepcb):
    """
    Test adding and removing categories with the category chooser dialog.

    Note: The full coverage of the category chooser dialog (expanding/
    collapsing, filtering, keyboard shortcuts, ...) is done in the symbol
    editor test (test_symbol_editor.py). This test only covers the basic
    usage of the dialog in the component editor to reduce redundancy.
    """
    librepcb.add_local_library_to_workspace("libraries/Populated Library.lplib")

    with librepcb.open() as app:
        # Start creating a new component with the wizard.
        app.get("SideBar::libraries-btn").click()
        app.get("LibrariesPanel::local-libs #LibraryListViewItem").dclick()
        app.get("LibraryContentTab::new-cmp-btn").click()
        dialog = app.get("ComponentTab::add-category-dialog ?")
        tree_items = dialog.get("TreeView::item-ta *")

        # Helpers
        def _open_dialog():
            app.get(
                "ComponentMetadataTab::categories-cell CategoriesRowCell::add-ta"
            ).click()
            dialog.wait(1)

        def _remove_category(category):
            # The remove button is only visible while the pointer hovers
            # over the corresponding row, so move the pointer there first.
            item = app.get(
                "ComponentMetadataTab::categories-cell CategoriesRowCell::item-ta *"
            ).wait_for_item(lambda item: item.label == category)
            app.hover(item.rect.center_abs)
            # Only the hovered row's remove button is visible.
            app.get(
                "ComponentMetadataTab::categories-cell CategoriesRowCell::item-remove-btn"
            ).click()

        def _verify_result(categories):
            dialog.wait_for(valid=False)
            app.get(
                "ComponentMetadataTab::categories-cell CategoriesRowCell::item-ta *"
            ).wait_for(label=categories)

        # Test 1:
        # Type a filter term, then pick the "LEDs" category with the mouse.
        # This closes the dialog and adds the category to the component.
        _open_dialog()
        dialog.get("TreeViewDialog::filter-edt").set_value("led")
        tree_items.wait_for(label=["Optoelectronics", "LEDs"])
        tree_items.wait_for_item(lambda item: item.label == "LEDs").click()
        _verify_result(["LEDs"])

        # Test 2:
        # Remove the category again with the mouse.
        _remove_category("LEDs")
        _verify_result([])

        # Test 3:
        # Open the dialog again and close it with the Escape key. No category
        # must be added when the dialog is closed this way.
        _open_dialog()
        app.trigger_shortcut([keys.Escape])
        _verify_result([])
