#!/usr/bin/env python
# -*- coding: utf-8 -*-

import time

"""
Test creating local libraries with the library manager
"""


def test(librepcb, helpers):
    """
    Create new local library
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

        # Create new library
        app.widget('libraryManagerAddLibraryTabBar').set_current_tab(1)
        widget_properties = {
            ('NameEdit', 'text'): 'Local Library',
            ('DescriptionEdit', 'text'): 'Foo Bar',
            ('AuthorEdit', 'text'): 'Functional Test',
            ('UrlEdit', 'text'): '',
            ('VersionEdit', 'text'): '1.2.3',
            ('Cc0LicenseCheckbox', 'checked'): True,
        }
        for (widget, property), value in widget_properties.items():
            app.widget('libraryManagerCreateLocalLibrary' + widget).set_property(property, value)
        time.sleep(0.5)  # Workaround for https://github.com/parkouss/funq/issues/39
        app.widget('libraryManagerCreateLocalLibraryCreateButton').click()

        # Check if one library is added
        library_count_after = library_count_before + 1
        helpers.wait_for_model_items_count(library_list, library_count_after,
                                           library_count_after)

        # Open the new library to check if everything is OK
        app.widget('libraryManagerLibraryInfoWidgetOpenEditorButton').click()
        assert app.widget('libraryEditor').properties()['visible'] is True
