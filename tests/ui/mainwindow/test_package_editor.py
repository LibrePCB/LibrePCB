#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the package editor
"""


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
