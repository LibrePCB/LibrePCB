#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the category editors
"""

import pytest
from slint_testing import keys


@pytest.mark.parametrize(
    "new_btn, filter_term, filtered_labels, parent",
    [
        # Component category: "LEDs" is a child of "Optoelectronics".
        (
            "LibraryContentTab::new-cmpcat-btn",  # New component category
            "led",
            ["Root Category", "Optoelectronics", "LEDs"],
            "LEDs",
        ),
        # Package category: The package categories in the test library are
        # flat (no subcategories), so a top-level category is used here.
        (
            "LibraryContentTab::new-pkgcat-btn",  # New package category
            "crystal",
            ["Root Category", "Crystals"],
            "Crystals",
        ),
    ],
    ids=["component", "package"],
)
def test_edit_parent_category(librepcb, new_btn, filter_term, filtered_labels, parent):
    """
    Test choosing the parent category of a new category with the category
    chooser dialog.

    Note: The full coverage of the category chooser dialog (expanding/
    collapsing, filtering, keyboard shortcuts, ...) is done in the symbol
    editor test (test_symbol_editor.py). This test only covers the basic
    usage of the dialog to reduce redundancy.
    """
    librepcb.add_local_library_to_workspace("libraries/Populated Library.lplib")

    with librepcb.open() as app:
        # Start creating a new category.
        app.get("SideBar::libraries-btn").click()
        app.get("LibrariesPanel::local-libs #LibraryListViewItem").dclick()
        app.get(new_btn).click()
        dialog = app.get("CategoryTab::choose-parent-dialog ?")
        tree_items = dialog.get("TreeView::item-ta *")

        # Helpers
        def _open_dialog():
            app.get("CategoryTab::parent-cell ParentRowCell::select-btn").click()
            dialog.wait(1)

        def _verify_result(parent):
            dialog.wait_for(valid=False)
            app.get("CategoryTab::parent-cell ParentRowCell::tree-txt").wait_for(
                label=parent
            )

        # Test 1:
        # Type a filter term, then pick the desired parent category with the
        # mouse. This closes the dialog and sets the parent of the category.
        _open_dialog()
        dialog.get("TreeViewDialog::filter-edt").set_value(filter_term)
        tree_items.wait_for(label=filtered_labels)
        tree_items.wait_for_item(lambda item: item.label == parent).click()
        _verify_result(parent)

        # Test 2:
        # Pick the "Root Category" to remove the parent again.
        _open_dialog()
        tree_items.wait_for_item(lambda item: item.label == "Root Category").click()
        _verify_result("Root Category")

        # Test 3:
        # Open the dialog again and close it with the Escape key. The parent
        # must not be changed.
        _open_dialog()
        app.trigger_shortcut([keys.Escape])
        _verify_result("Root Category")
