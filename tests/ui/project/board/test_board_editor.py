#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the board editor
"""

from slint_testing import keys


def test_status_bar_grid_interval_decimals(librepcb):
    """
    Regression test for https://github.com/LibrePCB/LibrePCB/issues/1853:
    The grid interval in the status bar shall always show all decimals.
    """
    librepcb.add_project("Empty Project")
    librepcb.set_project("Empty Project/Empty Project.lpp")
    with librepcb.open() as app:
        # Open board tab
        tabs = app.get("#TabButton *")
        tabs.wait_for_item(lambda x: x.label == "Board").click()
        app.get("#Board2dTab")

        # Decrease grid interval down to minimum and verify rendered value
        edit = app.get("StatusBar::grid-interval-edt")
        down_btn = app.get("StatusBar::grid-interval-decrease-btn")
        steps = ["2.54", "1.27", "0.635", "0.3175", "0.15875", "0.079375"]
        for expected in steps:
            edit.wait_for(value=expected)
            down_btn.click()
        edit.wait_for(value=steps[-1])  # Not dividable by 2 anymore

        # Put grid interval into edit mode and enter a value manually
        edit.click()
        app.trigger_shortcut(list("0.7654321") + [keys.Return])
        edit.wait_for(value="0.765432")

        # Switch to inches and verify the number has been converted
        app.get("StatusBar::unit-ta").click()
        units = app.get("StatusBar::unit-item-ta *")
        units.wait_for_item(lambda x: x.label == "inches").click()
        edit.wait_for(value="0.030135118")
