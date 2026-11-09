#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the package editor
"""

from slint_testing import keys


def test_edit_categories(librepcb):
    """
    Test adding and removing categories with the category chooser dialog.

    Note: The full coverage of the category chooser dialog (expanding/
    collapsing, filtering, keyboard shortcuts, ...) is done in the symbol
    editor test (test_symbol_editor.py). This test only covers the basic
    usage of the dialog in the package editor to reduce redundancy.
    """
    librepcb.add_local_library_to_workspace("libraries/Populated Library.lplib")

    with librepcb.open() as app:
        # Start creating a new package with the wizard.
        app.get("SideBar::libraries-btn").click()
        app.get("LibrariesPanel::local-libs #LibraryListViewItem").dclick()
        app.get("LibraryContentTab::new-pkg-btn").click()
        dialog = app.get("PackageTab::add-category-dialog ?")
        tree_items = dialog.get("TreeView::item-ta *")

        # Helpers
        def _open_dialog():
            app.get(
                "PackageMetadataTab::categories-cell CategoriesRowCell::add-ta"
            ).click()
            dialog.wait(1)

        def _remove_category(category):
            # The remove button is only visible while the pointer hovers
            # over the corresponding row, so move the pointer there first.
            item = app.get(
                "PackageMetadataTab::categories-cell CategoriesRowCell::item-ta *"
            ).wait_for_item(lambda item: item.label == category)
            app.hover(item.rect.center_abs)
            # Only the hovered row's remove button is visible.
            app.get(
                "PackageMetadataTab::categories-cell CategoriesRowCell::item-remove-btn"
            ).click()

        def _verify_result(categories):
            dialog.wait_for(valid=False)
            app.get(
                "PackageMetadataTab::categories-cell CategoriesRowCell::item-ta *"
            ).wait_for(label=categories)

        # Test 1:
        # Type a filter term, then pick the "Crystals" category with the
        # mouse. This closes the dialog and adds the category to the package.
        # Note: Unlike the other editors, the package editor uses *package*
        # categories (no subcategories in the test library), so a simple
        # top-level category is used here.
        _open_dialog()
        dialog.get("TreeViewDialog::filter-edt").set_value("crystal")
        tree_items.wait_for(label=["Crystals"])
        tree_items.wait_for_item(lambda item: item.label == "Crystals").click()
        _verify_result(["Crystals"])

        # Test 2:
        # Remove the category again with the mouse.
        _remove_category("Crystals")
        _verify_result([])

        # Test 3:
        # Open the dialog again and close it with the Escape key. No category
        # must be added when the dialog is closed this way.
        _open_dialog()
        app.trigger_shortcut([keys.Escape])
        _verify_result([])


def test_find_usages(librepcb):
    librepcb.add_local_library_to_workspace("libraries/Populated Library.lplib")
    with librepcb.open() as app:
        app.get("SideBar::libraries-btn").click()
        app.get("LibrariesPanel::local-libs #LibraryListViewItem").dclick()
        tabs = app.get("#TabButton *")

        # Open the R-0603 package
        categories = app.get(
            "LibraryContentTab::categories-view LibraryTreeView::item-ta *"
        )
        categories.wait(20, None)
        categories[19].click()  # R-SMT
        packages = app.get(
            "LibraryContentTab::elements-view LibraryTreeView::item-ta *"
        )
        packages.wait(3, None)
        packages[2].dclick()  # R-0603
        tabs.wait(3)
        assert tabs[-1].label == "R-0603"

        # Open the usages dialog and verify the listed device
        app.get("MainMenuBar::package-menu-item").click()
        app.get("MainMenuBar::find-package-usages-item").click()
        dialog = app.get("PackageTab::usages-dialog")
        usages = dialog.get("TreeView::item-ta *")
        usages.wait_for(label=["R-0603"])

        # Close the dialog
        dialog.get("OverlayDialog::close-btn").click()
        dialog.wait_for(valid=False)
