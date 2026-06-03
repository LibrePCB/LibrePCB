#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the device editor
"""

from slint_testing import keys


def test_open_package_and_component(librepcb):
    librepcb.add_local_library_to_workspace("libraries/Populated Library.lplib")
    with librepcb.open() as app:
        app.get("SideBar::libraries-btn").click()
        app.get("LibrariesPanel::local-libs #LibraryListViewItem").dclick()
        tabs = app.get("#TabButton *")

        # Open device
        items = app.get("LibraryContentTab::elements-view LibraryTreeView::item-ta *")
        items.wait(2, None)
        items[1].dclick()  # C-0805 device
        tabs.wait(3)
        assert tabs[-1].label == "C-0805"

        # Open package
        app.get(
            "DeviceContentTab::package-card DeviceDependencyCard::title-btn"
        ).click()
        tabs.wait(4)
        assert tabs[-1].label == "C-0805"
        tabs[-1].mclick()

        # Open component
        app.get(
            "DeviceContentTab::component-card-{1,2} DeviceDependencyCard::title-btn"
        ).click()
        tabs.wait(4)
        assert tabs[-1].label == "Capacitor Bipolar"
        tabs[-1].mclick()


# Regression test for a crash in LibrePCB 2.1.0.
def test_part_attributes_editor_cancel(librepcb):
    librepcb.add_local_library_to_workspace("libraries/Populated Library.lplib")
    with librepcb.open() as app:
        app.get("SideBar::libraries-btn").click()
        app.get("LibrariesPanel::local-libs #LibraryListViewItem").dclick()
        tabs = app.get("#TabButton *")

        # Open device
        items = app.get("LibraryContentTab::elements-view LibraryTreeView::item-ta *")
        items.wait(2, None)
        items[1].dclick()  # C-0805 device
        tabs.wait(3)
        assert tabs[-1].label == "C-0805"

        # Open attributes editor of new part and focus the key edit
        app.get("PartListView::attributes-ta").click()
        overlay = app.get("DeviceContentTab::attributes-overlay")
        overlay.get("AttributeListView::key-edt").click()

        # Close the attributes editor with the Escape key and verify that this
        # closes the overlay. This operation caused a crash in LibrePCB 2.1.0.
        app.trigger_shortcut([keys.Escape])
        overlay.wait_for(valid=False)
