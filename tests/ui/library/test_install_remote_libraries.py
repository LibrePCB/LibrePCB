#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import platform
import pytest
from slint_testing import keys, KeyPressedEvent, KeyReleasedEvent


"""
Test installing remote libraries with the library manager
"""

REMOTE_LIBRARY_COUNT = 3  # The dummy API provides 3 libraries


@pytest.mark.skipif(
    (os.name == "nt") or (platform.system() == "Darwin"),
    reason="strange error in Windows container and flaky on macOS",
)
def test_click(librepcb, helpers):
    with librepcb.open() as app:
        # Open libraries panel
        app.get("SideBar::libraries-btn").click()

        # Wait until all libraries are fetched
        libs = app.get("LibrariesPanel::remote-libs #LibraryListViewItem *")
        libs.wait(REMOTE_LIBRARY_COUNT)

        # Check the last library, which also checks dependent library 0
        switches = libs.get("#Switch *")
        switches.wait_for(checked=[False, False, False])
        switches[2].click()
        switches.wait_for(checked=[True, False, True])

        # Deselect the first library, which also unchecks dependent library 2
        switches[0].click()
        switches.wait_for(checked=[False, False, False])

        # Check the second library, which also checks dependent library 0
        switches[1].click()
        switches.wait_for(checked=[True, True, False])

        # Install selected libraries
        # Note: The button is animated, thus the click() doesn't work
        # reliably so we use trigger() instead.
        btn = app.get("LibrariesPanel::apply-btn")
        btn.wait_for(enabled=True)
        btn.trigger()

        # Verify that the libraries have been installed
        path = os.path.join(librepcb.workspace_path, "data/libraries/remote")
        dirs = [
            "a9ddf0c6-9b1c-4730-b300-01b4f192ad40.lplib",
            "6ccc516c-21b7-4cd5-9cf2-7a04cfa361c6.lplib",
        ]
        helpers.wait_for_directories(path, dirs)

        # Verify the status badge of installed libraries is now shown
        status = libs.get("LibraryListViewItem::status-btn *")
        status.wait(2)


@pytest.mark.skipif(platform.system() == "Darwin", reason="flaky on macOS")
def test_shortcut(librepcb, helpers):
    """
    Same as the test above, but using the Ctrl+Return keyboard shortcut to
    trigger the apply button since the test above doesn't work on Windows.
    """
    with librepcb.open() as app:
        # Open libraries panel
        app.get("SideBar::libraries-btn").click()

        # Wait until all libraries are fetched
        libs = app.get("LibrariesPanel::remote-libs #LibraryListViewItem *")
        libs.wait(REMOTE_LIBRARY_COUNT)

        # Check the last library, which also checks dependent library 0
        switches = libs.get("#Switch *")
        switches.wait_for(checked=[False, False, False])
        switches[2].click()
        switches.wait_for(checked=[True, False, True])

        # Deselect the first library, which also unchecks dependent library 2
        switches[0].click()
        switches.wait_for(checked=[False, False, False])

        # Check the second library, which also checks dependent library 0
        switches[1].click()
        switches.wait_for(checked=[True, True, False])

        # Install selected libraries
        panel = app.get("LibrariesPanel::remote-libs")
        panel.click()  # Set focus
        panel.window.dispatch_event(KeyPressedEvent(keys.Control))
        panel.window.dispatch_event(KeyPressedEvent(keys.Return))
        panel.window.dispatch_event(KeyReleasedEvent(keys.Return))
        panel.window.dispatch_event(KeyReleasedEvent(keys.Control))

        # Verify that the libraries have been installed
        path = os.path.join(librepcb.workspace_path, "data/libraries/remote")
        dirs = [
            "a9ddf0c6-9b1c-4730-b300-01b4f192ad40.lplib",
            "6ccc516c-21b7-4cd5-9cf2-7a04cfa361c6.lplib",
        ]
        helpers.wait_for_directories(path, dirs)

        # Verify the status badge of installed libraries is now shown
        status = libs.get("LibraryListViewItem::status-btn *")
        status.wait(2)
