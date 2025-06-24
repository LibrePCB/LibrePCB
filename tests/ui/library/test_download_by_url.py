#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import time

"""
Test downloading local libraries by URL
"""

URL = "http://localhost:50080/blobs/LibrePCB_Base.zip"


def test(librepcb, helpers):
    with librepcb.open() as app:
        # Open libraries panel
        app.get("SideBar::libraries-btn").click()

        # Verify that no local library exists yet
        libs = app.get("LibrariesPanel::local-libs #LibraryListViewItem *")
        assert libs.label == []

        # Create library
        app.get("LibrariesPanelSection::download-by-url-btn").click()
        tab = app.get("#DownloadLibraryTab")
        tab.get("DownloadLibraryTab::url-edt").set_value(URL)
        tab.get("DownloadLibraryTab::directory-edt").set_value("base.lplib")
        time.sleep(1)  # Give time to apply values to backend
        tab.get("DownloadLibraryTab::download-btn").click()

        # Verify that the tab has been closed
        tab.wait_for(valid=False)

        # Verify that the new library appears in the library list
        libs.wait(1)
        assert libs.label == ["LibrePCB Base"]

        # Open the new library to verify everything is OK by checking that
        # a lock file has been created
        libs[0].dclick()
        helpers.wait_for_file_exists(
            os.path.join(
                librepcb.workspace_path,
                "data/libraries/local/base.lplib/.lock",
            )
        )
