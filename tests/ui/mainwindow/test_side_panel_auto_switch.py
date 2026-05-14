#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test if the side panel automatically switches to the previously memorized
page when the current tab changes.
"""


def test_1(librepcb):
    librepcb.add_project("Empty Project")
    librepcb.set_project("Empty Project/Empty Project.lpp")
    with librepcb.open() as app:
        tabs = app.get("#TabButton *")
        tabs.wait(3)

        # Switch to ERC panel
        rule_check_btn = app.get("SideBar::rule-check-btn")
        rule_check_btn.wait_for(enabled=True)
        rule_check_btn.click()
        rule_check_btn.wait_for(checked=True)
        rule_check_header = app.get("RuleCheckPanel::header")
        rule_check_header.wait_for(label="Electrical Rule Check".upper())

        # Open board editor -> should still be on rule check panel (now DRC)
        tabs[2].click()
        rule_check_btn.wait_for(enabled=True, checked=True)
        rule_check_header.wait_for(label="Design Rule Check".upper())

        # Switch to place devices panel
        place_devices_btn = app.get("SideBar::place-devices-btn")
        place_devices_btn.click()
        place_devices_header = app.get("PlaceDevicesPanel::header")
        place_devices_header.wait_for(label="Place Devices".upper())

        # Switch back to schematic tab -> shall open the ERC panel
        tabs[1].click()
        rule_check_btn.wait_for(enabled=True, checked=True)
        rule_check_header.wait_for(label="Electrical Rule Check".upper())

        # Switch back to board tab -> shall open the place devices panel
        tabs[2].click()
        place_devices_btn.wait_for(enabled=True, checked=True)
        place_devices_header.wait_for(label="Place Devices".upper())


def test_2(librepcb):
    librepcb.add_project("Empty Project")
    librepcb.set_project("Empty Project/Empty Project.lpp")
    with librepcb.open() as app:
        tabs = app.get("#TabButton *")
        tabs.wait(3)

        # Open board editor
        tabs[2].click()

        # Switch to DRC panel
        rule_check_btn = app.get("SideBar::rule-check-btn")
        rule_check_btn.wait_for(enabled=True)
        rule_check_btn.click()
        rule_check_btn.wait_for(enabled=True, checked=True)
        rule_check_header = app.get("RuleCheckPanel::header")
        rule_check_header.wait_for(label="Design Rule Check".upper())

        # Switch to place devices panel
        place_devices_btn = app.get("SideBar::place-devices-btn")
        place_devices_btn.click()
        place_devices_header = app.get("PlaceDevicesPanel::header")
        place_devices_header.wait_for(label="Place Devices".upper())

        # Switch to schematic tab -> should go back to rule check panel
        # (now ERC) as this is probably the most intuitive behavior (better
        # than switching to the documents panel).
        tabs[1].click()
        rule_check_btn.wait_for(enabled=True, checked=True)
        rule_check_header = app.get("RuleCheckPanel::header")
        rule_check_header.wait_for(label="Electrical Rule Check".upper())
