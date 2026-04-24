#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the device editor
"""


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
