#!/usr/bin/env python
# -*- coding: utf-8 -*-

import time
import os
import pytest
from slint_testing import keys, KeyPressedEvent, KeyReleasedEvent


"""
Test installing remote libraries with the library manager
"""

REMOTE_LIBRARY_COUNT = 3  # The dummy API provides 3 libraries


def test_click(librepcb, helpers):
    with librepcb.open() as app:
        # Open libraries panel
        app.get("SideBar::libraries-btn").click()

        # Wait until all libraries are fetched
        libs = app.get("LibrariesPanel::remote-libs #LibraryListViewItem *")
        libs.wait(REMOTE_LIBRARY_COUNT)

        # Check the last library, which also checks dependent library 0
        switches = libs.get("#Switch *")

        for i in range(1, 100):
            # Check the second library, which also checks dependent library 0
            switches[1].click()
            switches.wait_for(checked=[True, True, False])

            # Deselect the first library, which also unchecks dependent library 2
            switches[0].click()
            switches.wait_for(checked=[False, False, False])

            print(i)
