#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

"""
Test creating local libraries
"""


def test(librepcb, helpers):
    with librepcb.open() as app:
        # Verify that no local library exists yet
        item = app.get('LibrariesPanel::local-libs #LibraryListViewItem')
        assert item.is_valid is False

        # Create library
        app.get('SideBar::libraries-btn').click()
        app.get('LibrariesPanelSection::create-btn').click()
        tab =  app.get('#CreateLibraryTab')
        tab.get('CreateLibraryTab::name-edt').set_value('Local Library')
        tab.get('CreateLibraryTab::description-edt').set_value('Foo Bar')
        tab.get('CreateLibraryTab::author-edt').set_value('Functional Test')
        tab.get('CreateLibraryTab::version-edt').set_value('1.2.3')
        tab.get('CreateLibraryTab::url-edt').set_value('')
        tab.get('CreateLibraryTab::cc0-sw').set_checked(True)
        tab.get('CreateLibraryTab::create-btn').click()

        # Verify that the tab has been closed
        helpers.wait_for_element_invalid(tab)

        # Verify that the new library appears in the library list
        assert item.label == "Local Library"

        # Open the new library to verify everything is OK by checking that
        # a lock file has been created
        item.dclick()
        helpers.wait_for_file_exists(
            os.path.join(librepcb.workspace_path, 'data', 'libraries', 'local', 'Local_Library.lplib', '.lock')
        )
