#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test window sections
"""


def test_close_first_section(librepcb):
    """
    When closing the first window section, the current tab on the second section
    must be kept even though the home tab moves to the second section. So if
    the current tab index on the second section was 'x', it needs to be changed
    to 'x+1'.
    """
    librepcb.add_project("Empty Project")
    librepcb.set_project("Empty Project/Empty Project.lpp")
    with librepcb.open() as app:
        # Move first tab to new section
        section = app.get("#WindowSection")
        tabs = app.get("#TabButton *")
        tabs.wait(3)
        app.drag_and_drop(tabs[1].rect.center_abs, section.rect.position_abs(0.91, 0.5))

        # Move second tab to new section
        tabs.wait(3)
        app.drag_and_drop(tabs[1].rect.center_abs, tabs[2].rect.position_abs(0.05, 0.5))

        # Verify state
        sections = app.get("#WindowSection *")
        sections.wait(2)
        tabs = app.get("#TabButton *")
        tabs.wait(3)
        tabs.wait_for(checked=True, indices=[1])

        # Close left section
        app.get("WindowSection::close-btn").click()
        sections.wait(1)

        # Verify that the active tab has not changed
        tabs.wait(3)
        tabs.wait_for(checked=True, indices=[1])
