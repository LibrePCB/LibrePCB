#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the project library manager
"""


def test(librepcb):
    librepcb.add_local_library_to_workspace("libraries/Empty Library.lplib")
    librepcb.add_project("Gerber Test")
    librepcb.set_project("Gerber Test/project.lpp")
    with librepcb.open() as app:
        # Open library manager
        app.get("MainMenuBar::project-menu-item").click()
        app.get("MainMenuBar::project-library-manager-item").click()

        # Verify that the tab has been opened
        tabs = app.get("#TabButton *")
        tabs.wait(4)
        assert tabs[-1].label == "test_project"
        assert tabs[-1].checked
        tab = app.get("#ProjectLibraryTab")

        # Check some items
        checkboxes = app.get("#ProjectLibraryTab ProjectLibraryListView::checked-sw *")
        checkboxes.wait(8)
        checkboxes[1].click()
        checkboxes[3].click()

        # Trigger "copy to workspace"
        copy_btn = app.get("ProjectLibraryTab::copy-btn")
        copy_btn.wait_for(enabled=True)
        copy_btn.click()
        app.get("ChooseLocalLibraryDialog::item-ta").click()

        # Verify that the library is now set on the copied elements
        libraries = app.get("#ProjectLibraryTab ProjectLibraryListView::library-txt *")
        libraries.wait(5, None)
        libraries.wait_for(indices=[1, 3], label="Empty Library")

        # Verify that the checkbox has disappeared for the copied elements
        checkboxes.wait(6)

        # Switch tabs
        tabs[1].click()
        tab.wait_for(valid=False)
        tabs[-1].click()
        tab.wait_for(valid=True)

        # Close tab
        app.get("ProjectLibraryTab::close-btn").click()
        tabs.wait(3)

        # Open tab again, then close project while the tab is open
        app.get("MainMenuBar::project-menu-item").click()
        app.get("MainMenuBar::project-library-manager-item").click()
        tabs.wait(4)
        app.get("DocumentsPanel::project-item ProjectSection::close-btn").click()
        tabs.wait(1)
