#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test switching tabs
"""


def test_new_library_element_tabs(librepcb, helpers):
    """
    This is a reproducer of a real bug. The application crashed with
    std::bad_optional_access when switching from a "New Component" tab back
    to the library overview tab.
    """
    librepcb.add_local_library_to_workspace("libraries/Populated Library.lplib")
    buttons = [
        ["LibraryContentTab::new-cmpcat-btn"],
        ["LibraryContentTab::new-pkgcat-btn"],
        ["LibraryContentTab::new-sym-btn"],
        ["LibraryContentTab::new-pkg-btn"],
        ["LibraryContentTab::new-cmp-btn"],
        ["LibraryContentTab::new-dev-btn"],
        ["MainMenuBar::library-menu-item", "MainMenuBar::new-organization-item"],
    ]

    with librepcb.open() as app:
        app.get("SideBar::libraries-btn").click()
        app.get("LibrariesPanel::local-libs #LibraryListViewItem").dclick()
        for i, button in enumerate(buttons):
            for item in button:
                app.get(item).trigger()  # Opens a new tab to create a new element
            tabs = app.get("#TabButton *")
            tabs.wait(i + 3)
            assert tabs[-1].checked
            if len(button):
                tabs[1].click()  # Additional click is required for closing menu
            tabs[1].click()  # Switch back to library overview
            assert tabs[1].checked
            tabs[-1].click()  # Switch back to new element tab
            assert tabs[-1].checked
            tabs[1].click()  # Switch back to library overview
            assert tabs[1].checked

        # Sweep through all tabs
        for i in range(len(buttons) + 2):
            tabs[i].click()
            assert tabs[i].checked


def test_existing_library_element_tabs(librepcb, helpers):
    librepcb.add_local_library_to_workspace("libraries/Populated Library.lplib")
    elements = [
        dict(
            items="LibraryContentTab::categories-view LibraryTreeView::item-ta *",
            category=0,
            index=3,
            name="Capacitors",  # Component category
        ),
        dict(
            items="LibraryContentTab::categories-view LibraryTreeView::item-ta *",
            category=0,
            index=18,
            name="R-SMT",  # Package category
        ),
        dict(
            items="LibraryContentTab::elements-view LibraryTreeView::item-ta *",
            category=3,  # Capacitors
            index=1,
            name="C-0805",  # Device
        ),
        dict(
            items="LibraryContentTab::elements-view LibraryTreeView::item-ta *",
            category=3,  # Capacitors
            index=3,
            name="Capacitor Bipolar",  # Component
        ),
        dict(
            items="LibraryContentTab::elements-view LibraryTreeView::item-ta *",
            category=3,  # Capacitors
            index=6,
            name="Capacitor Bipolar EU",  # Symbol
        ),
        dict(
            items="LibraryContentTab::elements-view LibraryTreeView::item-ta *",
            category=18,  # R-SMT
            index=2,
            name="R-0603",  # Package
        ),
        dict(
            items="LibraryContentTab::elements-view LibraryTreeView::item-ta *",
            category=21,  # Organizations
            index=1,
            name="AISLER",  # Organization
        ),
    ]

    with librepcb.open() as app:
        app.get("SideBar::libraries-btn").click()
        app.get("LibrariesPanel::local-libs #LibraryListViewItem").dclick()
        for i, element in enumerate(elements):
            # Open the library overview
            tabs = app.get("#TabButton *")
            tabs.wait(i + 2)
            tabs[1].click()

            # Choose the corresponding category
            categories = app.get(
                "LibraryContentTab::categories-view LibraryTreeView::item-ta *"
            )
            categories.wait(element["category"] + 1, None)
            categories[element["category"]].click()

            # Open the element
            items = app.get(element["items"])
            items.wait(element["index"] + 1, None)
            items[element["index"]].dclick()

            # Verify the tab has been opened
            tabs.wait(i + 3)
            assert tabs[i + 2].label == element["name"]
            assert tabs[i + 2].checked

        # Sweep through all tabs
        for i in range(len(elements) + 2):
            tabs[i].click()
            assert tabs[i].checked
