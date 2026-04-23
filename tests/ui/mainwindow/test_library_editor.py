#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the library editor
"""


def test_close_tab_with_reverted_changes(librepcb):
    librepcb.add_local_library_to_workspace("libraries/Populated Library.lplib")
    with librepcb.open() as app:
        app.get("SideBar::libraries-btn").click()
        app.get("LibrariesPanel::local-libs #LibraryListViewItem").dclick()
        tabs = app.get("#TabButton *")
        tabs.wait(2)

        # Open a device to keep the library open.
        items = app.get("LibraryContentTab::elements-view LibraryTreeView::item-ta *")
        items.wait(2, None)
        items[1].dclick()
        tabs.wait(3)
        tabs[1].click()

        # If compact mode is used, switch to the metadata tab.
        app.get("#LibraryTab")  # Wait for the tab before accessing NavBar.
        nav_items = app.get("LibraryTab::navbar NavBar::item-btn *")
        if len(nav_items) > 0:
            nav_items[0].click()

        # Edit description and move focus to apply it.
        app.get("LibraryMetadataTab::name-edt").click()
        app.get("LibraryMetadataTab::name-edt").set_value("test")
        app.get("LibraryMetadataTab::description-edt").click()
        app.get("MainMenuBar::undo-btn").wait_for(enabled=True)
        app.get("MainMenuBar::redo-btn").wait_for(enabled=False)

        # Trigger undo and verify the change.
        app.get("MainMenuBar::undo-btn").click()
        app.get("LibraryMetadataTab::name-edt").wait_for(value="Populated Library")
        app.get("MainMenuBar::undo-btn").wait_for(enabled=False)
        app.get("MainMenuBar::redo-btn").wait_for(enabled=True)

        # Close the tab shall now be possible without asking for saving.
        tabs[1].mclick()
        tabs.wait(2)

        # When opening the library tab again, the undo stack must be cleared.
        app.get("#DocumentsPanel LibrarySection::overview-item").click()
        app.get("#LibraryTab")  # Wait for the tab before accessing NavBar.
        nav_items.wait(0, None)
        if len(nav_items) > 0:
            nav_items[0].click()
        app.get("LibraryMetadataTab::name-edt").wait_for(value="Populated Library")
        app.get("MainMenuBar::undo-btn").wait_for(enabled=False)
        app.get("MainMenuBar::redo-btn").wait_for(enabled=False)
