#!/usr/bin/env python
# -*- coding: utf-8 -*-

import time


"""
Test the various library update modes
"""

REMOTE_LIBRARY_COUNT = 3  # The dummy API provides 3 libraries


def test_disabled(librepcb, helpers):
    librepcb.set_library_update_mode("disabled")
    librepcb.add_remote_library_to_workspace("server/blobs/LibrePCB_Connectors.zip")
    with librepcb.open() as app:
        # Open libraries panel.
        app.get("SideBar::libraries-btn").click()

        # Library update would happen within 1 second, so we wait a bit longer.
        time.sleep(2)

        # Ensure that no sidebar emblem or notification is shown.
        app.get("SideBar::libraries-btn").wait_for(value="")
        helpers.wait_for_notification(app, "Library Updates Available", visible=False)

        # Ensure that only the installed library is listed.
        libs = app.get("LibrariesPanel::remote-libs #LibraryListViewItem *")
        libs.wait(1)


def test_check(librepcb, helpers):
    librepcb.set_library_update_mode("check")
    librepcb.add_remote_library_to_workspace(
        "server/blobs/LibrePCB_Base.zip", mark_outdated=True
    )
    with librepcb.open() as app:
        # Wait for the sidebar emblem to appear.
        app.get("SideBar::libraries-btn").wait_for(value="1")

        # Open libraries panel and check if remote libraries are listed.
        app.get("SideBar::libraries-btn").click()
        libs = app.get("LibrariesPanel::remote-libs #LibraryListViewItem *")
        libs.wait(REMOTE_LIBRARY_COUNT)

        # Check that no notification is shown.
        helpers.wait_for_notification(app, "Library Updates Available", visible=False)

        # Check emblem again, just to be sure no update was started.
        app.get("SideBar::libraries-btn").wait_for(value="1")


def test_notify(librepcb, helpers):
    librepcb.set_library_update_mode("notify")
    librepcb.add_remote_library_to_workspace(
        "server/blobs/LibrePCB_Base.zip", mark_outdated=True
    )
    helpers.wait_for_directories(
        librepcb.get_workspace_libraries_path("remote"),
        ["LibrePCB_Base.lplib"],
    )
    with librepcb.open() as app:
        # Wait for the sidebar emblem and notification to appear.
        app.get("SideBar::libraries-btn").wait_for(value="1")
        notification = helpers.wait_for_notification(app, "Library Updates Available")
        app.get("SideBar::libraries-btn").wait_for(value="1")

        # Click button to start the update and check that it disappears.
        notification.trigger()
        helpers.wait_for_notification(app, "Library Updates Available", visible=False)

        # Wait until the update has been installed. Since the original library
        # directory is not named by UUID, the directory will get renamed.
        helpers.wait_for_directories(
            librepcb.get_workspace_libraries_path("remote"),
            ["a9ddf0c6-9b1c-4730-b300-01b4f192ad40.lplib"],
        )


def test_install(librepcb, helpers):
    librepcb.set_library_update_mode("install")
    librepcb.add_remote_library_to_workspace(
        "server/blobs/LibrePCB_Base.zip", mark_outdated=True
    )
    helpers.wait_for_directories(
        librepcb.get_workspace_libraries_path("remote"),
        ["LibrePCB_Base.lplib"],
    )
    with librepcb.open() as app:
        # Wait until the update has been installed. Since the original library
        # directory is not named by UUID, the directory will get renamed.
        helpers.wait_for_directories(
            librepcb.get_workspace_libraries_path("remote"),
            ["a9ddf0c6-9b1c-4730-b300-01b4f192ad40.lplib"],
        )

        # Check that no notification is shown.
        helpers.wait_for_notification(app, "Library Updates Available", visible=False)
        app.get("SideBar::libraries-btn").wait_for(value="")


def test_install_with_errors(librepcb, helpers):
    librepcb.set_library_update_mode("install")
    librepcb.add_remote_library_to_workspace(
        "server/blobs/LibrePCB_Base.zip", mark_outdated=True
    )
    # Create a file with the name of the library to be installed. This will
    # cause the update to fail.
    libpath = librepcb.get_workspace_libraries_path(
        "remote/a9ddf0c6-9b1c-4730-b300-01b4f192ad40.lplib"
    )
    with open(libpath, "w") as f:
        f.write("")
    for i in range(5):
        with librepcb.open() as app:
            # Wait for the status emblem.
            app.get("SideBar::libraries-btn").wait_for(value="1")

            # Check that a notification is shown after 3 failures.
            helpers.wait_for_notification(
                app, "Library Updates Available", visible=(i >= 3)
            )
