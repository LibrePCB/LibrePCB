#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import pytest
import sys

"""
Test manually downloading libraries with the library manager
"""


@pytest.mark.xfail(sys.platform == "darwin",
                   reason="Test fails on macOS for an unknown reason.")
def test(librepcb, helpers):
    """
    Download library by URL
    """
    with librepcb.open() as app:

        # Open library manager
        app.widget('controlPanelOpenLibraryManagerButton').click()
        assert app.widget('libraryManager').properties()['visible'] is True

        # Make sure there is only one entry ("New Library") in the libraries list
        library_count_before = 1
        library_list = app.widget('libraryManagerInstalledLibrariesList')
        helpers.wait_for_model_items_count(library_list, library_count_before,
                                           library_count_before)

        # Make sure the target directory does not exist yet
        libdir = librepcb.get_workspace_libraries_path('local/base.lplib')
        assert not os.path.exists(libdir)

        # Download library
        app.widget('libraryManagerAddLibraryTabBar').set_current_tab(2)
        widget_properties = {
            ('UrlEdit', 'text'): 'http://localhost:50080/blobs/LibrePCB_Base.zip',
            ('DirectoryEdit', 'text'): 'base.lplib',
        }
        for (widget, property), value in widget_properties.items():
            app.widget('libraryManagerDownloadManually' + widget).set_property(property, value)
        app.widget('libraryManagerDownloadManuallyDownloadButton').click()

        # Check if one library is added
        library_count_after = library_count_before + 1
        helpers.wait_for_model_items_count(library_list, library_count_after,
                                           library_count_after)

        # Check if the library directory now exists
        assert os.path.exists(libdir)

        # Open the new library to check if everything is OK
        app.widget('libraryManagerLibraryInfoWidgetOpenEditorButton').click()
        assert app.widget('libraryEditor').properties()['visible'] is True
