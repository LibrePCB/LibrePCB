#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import time

"""
Test creating local libraries
"""


def test(librepcb, helpers):
    with librepcb.open() as app:
        # Open libraries panel
        app.get("SideBar::libraries-btn").click()

        # Verify that no local library exists yet
        libs = app.get("LibrariesPanel::local-libs #LibraryListViewItem *")
        assert libs.label == []

        # Create library
        app.get("LibrariesPanelSection::create-btn").click()
        tab = app.get("#CreateLibraryTab")
        tab.get("CreateLibraryTab::name-edt").set_value("Local Library")
        tab.get("CreateLibraryTab::description-edt").set_value("Foo Bar")
        tab.get("CreateLibraryTab::author-edt").set_value("Functional Test")
        tab.get("CreateLibraryTab::version-edt").set_value("1.2.3")
        tab.get("CreateLibraryTab::url-edt").set_value("")
        tab.get("CreateLibraryTab::cc0-sw").set_checked(True)
        directory_edt = tab.get("CreateLibraryTab::directory-edt")
        directory_edt.wait_for(placeholder="Local_Library.lplib")
        directory_edt.set_value("local-lib.lplib")
        time.sleep(1)  # Give time to apply values to backend
        tab.get("CreateLibraryTab::create-btn").click()

        # Verify that the tab has been closed
        tab.wait_for(valid=False)

        # Verify that the new library appears in the library list
        libs.wait(1)
        assert libs.label == ["Local Library"]

        # Open the new library to verify everything is OK by checking that
        # a lock file has been created
        libs[0].dclick()
        helpers.wait_for_file_exists(
            os.path.join(
                librepcb.workspace_path,
                "data/libraries/local/local-lib.lplib/.lock",
            )
        )
