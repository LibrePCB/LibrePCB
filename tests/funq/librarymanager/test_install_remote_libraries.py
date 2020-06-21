#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pytest
import sys

"""
Test installing remote libraries with the library manager
"""


@pytest.mark.xfail(sys.platform == "darwin",
                   reason="Test fails on macOS for an unknown reason.")
def test(librepcb, helpers):
    """
    Install some remote libraries
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

        # Wait until all libraries are fetched and check the count of them
        remote_library_count = 3  # The dummy API provides 3 libraries
        remote_library_list = app.widget('libraryManagerDownloadFromRepoLibraryList')
        helpers.wait_for_model_items_count(remote_library_list,
                                           remote_library_count,
                                           remote_library_count)

        # Get the required widgets of all libraries and check their initial state
        statuslabels = list()
        checkboxes = list()
        progressbars = list()
        for i in range(0, remote_library_count):
            suffix = "-{}".format(i) if i > 0 else ''
            item_path = app.aliases['libraryManagerDownloadFromRepoLibraryListItem'] + suffix
            statuslabel = app.widget(path=item_path + '::lblInstalledVersion')
            assert statuslabel.properties()['text'] == 'Recommended'
            statuslabels.append(statuslabel)
            checkbox = app.widget(path=item_path + '::cbxDownload')
            assert checkbox.properties()['checked'] is False
            checkboxes.append(checkbox)
            progressbar = app.widget(path=item_path + '::prgProgress', wait_active=False)
            assert progressbar.properties()['value'] == 0
            progressbars.append(progressbar)

        # Select second library to install -> this must also check the first
        # library because it's a dependency!
        checkboxes[1].set_property('checked', True)

        # Verify checked state of all libraries
        for i in range(0, remote_library_count):
            assert checkboxes[i].properties()['checked'] is (True if i <= 1 else False)

        # Install selected libraries
        app.widget('libraryManagerDownloadFromRepoDownloadButton').click()

        # Check if two libraries were added
        library_count_after = library_count_before + 2
        helpers.wait_for_model_items_count(library_list, library_count_after,
                                           library_count_after)

        # Wait until progress bars are at 100% and hidden (i.e. installation finished)
        for i in range(0, remote_library_count):
            props = {'value': 100 if i <= 1 else 0, 'visible': False}
            progressbars[i].wait_for_properties(props=props)

        # Check installed status of libraries in remote library list
        for i in range(0, remote_library_count):
            if i <= 1:
                # The downloaded libraries are outdated, thus the installed
                # version number is displayed.
                assert statuslabels[i].properties()['text'].startswith('v')
            else:
                assert statuslabels[i].properties()['text'] == 'Recommended'
