#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import time

"""
Test creating local libraries
"""


def test_default_values(librepcb, helpers):
    """
    Test with keeping all the default values.
    """
    expected_name = "My Library"
    expected_dirname = "My_Library.lplib"

    with librepcb.open() as app:
        # Open libraries panel
        app.get("SideBar::libraries-btn").click()

        # Verify that no local library exists yet
        libs = app.get("LibrariesPanel::local-libs #LibraryListViewItem *")
        assert libs.label == []

        # Create library
        app.get("LibrariesPanelSection::create-btn").click()
        tab = app.get("#CreateLibraryTab")
        tab.get("CreateLibraryTab::name-edt").wait_for(value=expected_name)
        tab.get("CreateLibraryTab::directory-edt").wait_for(
            placeholder=expected_dirname
        )
        time.sleep(1)  # Give time to apply values to backend
        tab.get("CreateLibraryTab::create-btn").click()

        # Verify that the tab has been closed
        tab.wait_for(valid=False)

        # Verify that the new library has been opened by checking that
        # a lock file has been created
        helpers.wait_for_file_exists(
            os.path.join(
                librepcb.workspace_path,
                f"data/libraries/local/{expected_dirname}/.lock",
            )
        )

        # Verify the library has been opened in a new tab
        tab = app.get("#LibraryTab")
        assert tab.get("LibraryMetadataTab::name-edt").value == expected_name


def test_auto_dirname(librepcb, helpers):
    """
    Test with keeping the auto-generated directory name.
    """
    name = "Local Library / With Very Long Name ? And Invalid Characters"
    expected_dirname = "Local_Library__With_Very_Long_Name__And_Inva.lplib"

    with librepcb.open() as app:
        # Open libraries panel
        app.get("SideBar::libraries-btn").click()

        # Verify that no local library exists yet
        libs = app.get("LibrariesPanel::local-libs #LibraryListViewItem *")
        assert libs.label == []

        # Create library
        app.get("LibrariesPanelSection::create-btn").click()
        tab = app.get("#CreateLibraryTab")
        tab.get("CreateLibraryTab::name-edt").set_value(name)
        tab.get("CreateLibraryTab::description-edt").set_value("Foo Bar")
        tab.get("CreateLibraryTab::author-edt").set_value("Functional Test")
        tab.get("CreateLibraryTab::version-edt").set_value("1.2.3")
        tab.get("CreateLibraryTab::url-edt").set_value("")
        tab.get("CreateLibraryTab::cc0-sw").set_checked(True)
        directory_edt = tab.get("CreateLibraryTab::directory-edt")
        directory_edt.wait_for(placeholder=expected_dirname)
        time.sleep(1)  # Give time to apply values to backend
        tab.get("CreateLibraryTab::create-btn").click()

        # Verify that the tab has been closed
        tab.wait_for(valid=False)

        # Verify that the new library has been opened by checking that
        # a lock file has been created
        helpers.wait_for_file_exists(
            os.path.join(
                librepcb.workspace_path,
                f"data/libraries/local/{expected_dirname}/.lock",
            )
        )

        # Verify the library has been opened in a new tab
        tab = app.get("#LibraryTab")
        assert tab.get("LibraryMetadataTab::name-edt").value == name


def test_manual_dirname(librepcb, helpers):
    """
    Test with overriding the auto-generated directory name.
    """
    name = "Local Library / With Very Long Name ? And Invalid Characters"
    set_dirname = (
        "Local Library / With Very Long Manual Name ? And Invalid Characters.lplib"
    )
    expected_dirname = "Local_Library__With_Very_Long_Manual_Name__A.lplib"

    with librepcb.open() as app:
        # Open libraries panel
        app.get("SideBar::libraries-btn").click()

        # Verify that no local library exists yet
        libs = app.get("LibrariesPanel::local-libs #LibraryListViewItem *")
        assert libs.label == []

        # Create library
        app.get("LibrariesPanelSection::create-btn").click()
        tab = app.get("#CreateLibraryTab")
        tab.get("CreateLibraryTab::name-edt").set_value(name)
        tab.get("CreateLibraryTab::description-edt").set_value("Foo Bar")
        tab.get("CreateLibraryTab::author-edt").set_value("Functional Test")
        tab.get("CreateLibraryTab::version-edt").set_value("1.2.3")
        tab.get("CreateLibraryTab::url-edt").set_value("")
        tab.get("CreateLibraryTab::cc0-sw").set_checked(True)
        directory_edt = tab.get("CreateLibraryTab::directory-edt")
        directory_edt.set_value(set_dirname)
        time.sleep(1)  # Give time to apply values to backend
        tab.get("CreateLibraryTab::create-btn").click()

        # Verify that the tab has been closed
        tab.wait_for(valid=False)

        # Verify that the new library has been opened by checking that
        # a lock file has been created
        helpers.wait_for_file_exists(
            os.path.join(
                librepcb.workspace_path,
                f"data/libraries/local/{expected_dirname}/.lock",
            )
        )

        # Verify the library has been opened in a new tab
        tab = app.get("#LibraryTab")
        assert tab.get("LibraryMetadataTab::name-edt").value == name
