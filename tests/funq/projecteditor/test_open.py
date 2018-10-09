#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test opening the project editor
"""


def test_window_titles(project_editor):
    """
    Check window titles of schematic and board editor
    """
    se = project_editor.widget('schematicEditor')
    be = project_editor.widget('boardEditor')
    assert se.properties()['windowTitle'] == 'Empty Project.lpp - LibrePCB Schematic Editor'
    assert be.properties()['windowTitle'] == 'Empty Project.lpp - LibrePCB Board Editor'
