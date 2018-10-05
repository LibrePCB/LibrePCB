#!/usr/bin/env python
# -*- coding: utf-8 -*-

import time

"""
Test opening projects by command line arguments
"""

project = 'data/fixtures/Empty Project/Empty Project.lpp'


def test_open_project(librepcb):
    """
    Open project by command line argument
    """
    librepcb.set_project(project)
    with librepcb.open() as app:
        # Check if both editors were opened
        assert app.widget('schematicEditor').properties()['visible'] is True
        assert app.widget('boardEditor').properties()['visible'] is True

        # Check if the schematic editor is the active window (first wait a bit
        # to give the window manager some time to order windows properly).
        time.sleep(0.2)
        active_widget = app.active_widget()
        print("Top window: " + str(active_widget.properties() if active_widget else None))
        assert active_widget.oid == app.widget('schematicEditor').oid
