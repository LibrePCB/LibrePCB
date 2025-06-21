#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pytest

library = "libraries/Populated Library.lplib"


@pytest.fixture
def library_editor(librepcb, helpers):
    """
    Fixture opening the library editor with an empty library
    """
    librepcb.add_local_library_to_workspace(path=library)
    with librepcb.open() as app:
        # Wait until the library scan is finished
        helpers.wait_for_library_scan_complete(app)

        # Open library editor of empty library
        err = app.widget("mainWindowTestAdapter").call_slot(
            "openLibraryEditor", "local/Populated Library.lplib"
        )
        if err:
            raise Exception(err)
        assert app.widget("libraryEditor").properties()["visible"] is True

        # Start the actual test
        yield app
