#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test installing remote libraries with the library manager
"""

REMOTE_LIBRARY_COUNT = 3  # The dummy API provides 3 libraries


def test(librepcb, helpers):
    with librepcb.open() as app:
        # Open libraries panel
        app.get('SideBar::libraries-btn').click()

        # Wait until all libraries are fetched
        libs = app.get('LibrariesPanel::remote-libs #LibraryListViewItem *')
        libs.wait(REMOTE_LIBRARY_COUNT)

        # Check the last library, which also checks dependent library 0
        switches = libs.get('#Switch *')
        assert switches.checked == [False, False, False]
        switches[2].click()
        assert switches.checked == [True, False, True]

        # Deselect the first library, which also unchecks dependent library 2
        switches[0].click()
        assert switches.checked == [False, False, False]

        # Check the second library, which also checks dependent library 0
        switches[1].click()
        assert switches.checked == [True, True, False]

        # Install selected libraries
        app.get('LibrariesPanel::apply-btn').wait_for_enabled().click()

#        app.widget('libraryManagerInstallLibrariesDownloadButton').click()
#
#        # Check if two libraries were added
#        library_count_after = library_count_before + 2
#        helpers.wait_for_model_items_count(library_list, library_count_after,
#                                           library_count_after)
#
#        # Wait until progress bars are at 100% and hidden (i.e. installation finished)
#        for i in range(0, remote_library_count):
#            props = {'value': 100 if i <= 1 else 0, 'visible': False}
#            progressbars[i].wait_for_properties(props=props)
#
#        # Check installed status of libraries in remote library list
#        for i in range(0, remote_library_count):
#            if i <= 1:
#                # The downloaded libraries are outdated, thus the installed
#                # version number is displayed.
#                assert statuslabels[i].properties()['text'].startswith('v')
#            else:
#                assert statuslabels[i].properties()['text'] == 'Recommended'

        import time; time.sleep(3)
