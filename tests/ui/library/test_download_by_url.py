#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

"""
Test downloading local libraries by URL
"""

URL = 'http://localhost:50080/blobs/LibrePCB_Base.zip'


def test(librepcb, helpers):
    with librepcb.open() as app:
        # Verify that no local library exists yet
        item = app.get('LibrariesPanel::local-libs #LibraryListViewItem')
        assert item.is_valid is False

        # Create library
        app.get('SideBar::libraries-btn').click()
        app.get('LibrariesPanelSection::download-by-url-btn').click()
        tab =  app.get('#DownloadLibraryTab')
        tab.get('DownloadLibraryTab::url-edt').set_value(URL)
        tab.get('DownloadLibraryTab::directory-edt').set_value('base.lplib')
        tab.get('DownloadLibraryTab::download-btn').click()

        # Verify that the tab has been closed
        helpers.wait_for_element_invalid(tab)

        # Verify that the new library appears in the library list
        assert item.label == "LibrePCB Base"

        # Open the new library to verify everything is OK by checking that
        # a lock file has been created
        item.dclick()
        helpers.wait_for_file_exists(
            os.path.join(librepcb.workspace_path, 'data', 'libraries', 'local', 'base.lplib', '.lock')
        )
