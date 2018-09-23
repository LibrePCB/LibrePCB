#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test installing remote libraries with the library manager
"""


def test(librepcb):
    """
    Install some remote libraries
    """
    with librepcb.open() as app:

        # Open library manager
        app.widget('controlPanelOpenLibraryManagerButton').click()
        assert app.widget('libraryManager').properties()['visible'] is True

        # Make sure there is only one entry ("New Library") in the libraries list
        library_list = app.widget('libraryManagerInstalledLibrariesList')
        library_count_before = len(library_list.model_items().items)
        assert library_count_before == 1

        # TODO: Wait until all libraries are fetched and check the count of them
        remote_library_count = 3  # The dummy API provides 3 libraries

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

        # Wait until progress bars are at 100% and hidden (i.e. installation finished)
        for i in range(0, remote_library_count):
            props = {'value': 100 if i <= 1 else 0, 'visible': False}
            progressbars[i].wait_for_properties(props=props)

        # Check installed status of libraries in remote library list
        for i in range(0, remote_library_count):
            if i <= 1:
                assert statuslabels[i].properties()['text'].startswith('Installed')
            else:
                assert statuslabels[i].properties()['text'] == 'Recommended'

        # Check if exactly two libraries were added to the library list
        library_count_after = len(library_list.model_items().items)
        assert library_count_after == library_count_before + 2
