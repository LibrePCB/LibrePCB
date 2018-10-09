#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pytest

library = 'data/fixtures/Populated Library.lplib'


@pytest.fixture
def library_editor(librepcb):
    """
    Fixture opening the library editor with an empty library
    """
    librepcb.add_local_library_to_workspace(path=library)
    with librepcb.open() as app:
        # Open library manager
        app.widget('controlPanelOpenLibraryManagerButton').click()
        assert app.widget('libraryManager').properties()['visible'] is True

        # Select the empty library in library list
        library_list = app.widget('libraryManagerInstalledLibrariesList')
        assert len(library_list.model_items().items) == 2
        library_item = library_list.model_items().items[1]
        library_item.select()

        # Open library editor of empty library
        app.widget('libraryManagerLibraryInfoWidgetOpenEditorButton').click()
        assert app.widget('libraryEditor').properties()['visible'] is True

        # Start the actual test
        yield app
