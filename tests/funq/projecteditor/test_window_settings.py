#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pytest

"""
Test if window settings (position, size, toolbar visibility, ...) are saved and
restored properly.
"""


def close_application(app):
    pass  # nothing to do, it will be closed automatically


def close_project_action(app):
    app.action('schematicEditorActionCloseProject').trigger()


def close_schematic_editor_first(app):
    app.widget('schematicEditor').close()
    app.widget('boardEditor').close()


def close_board_editor_first(app):
    app.widget('boardEditor').close()
    app.widget('schematicEditor').close()


@pytest.mark.parametrize('close_method', [
    close_application,
    close_project_action,
    close_schematic_editor_first,
    close_board_editor_first,
])
def test(librepcb, close_method):
    """
    Check settings with multiple variants how the windows can be closed:
      - Whole application closed by the OS
      - Project closed with "Close Project" tool button
      - Schematic and board editor windows closed, in both orders

    This is needed to check if settings are saved/restored properly in all
    cases.
    """
    new_sch_pos = (50, 60)
    new_sch_size = (810, 610)
    new_brd_pos = (70, 80)
    new_brd_size = (820, 620)

    librepcb.add_project('Empty Project')
    librepcb.set_project('Empty Project/Empty Project.lpp')
    with librepcb.open() as app:
        # check default settings (LibrePCB.ini did not exist yet)
        assert app.widget('schematicEditorToolbarFile').properties()['visible'] is True
        assert app.widget('schematicEditorDockErcMsg').properties()['visible'] is True
        assert app.widget('boardEditorToolbarFile').properties()['visible'] is True
        assert app.widget('boardEditorDockErcMsg').properties()['visible'] is True

        # change some settings
        new_sch_pos = app.widget('schematicEditor').move(*new_sch_pos)
        new_sch_size = app.widget('schematicEditor').resize(*new_sch_size)
        new_brd_pos = app.widget('boardEditor').move(*new_brd_pos)
        new_brd_size = app.widget('boardEditor').resize(*new_brd_size)

        # close application with specified method
        close_method(app)

    # restart librepcb
    with librepcb.open() as app:
        # check settings
        assert app.widget('schematicEditorToolbarFile').properties()['visible'] is True
        assert app.widget('schematicEditorDockErcMsg').properties()['visible'] is True
        assert app.widget('boardEditorToolbarFile').properties()['visible'] is True
        assert app.widget('boardEditorDockErcMsg').properties()['visible'] is True

        # Note: Window positions aren't checked because the window manager
        # might place the window on another position and thus that test would
        # not be reliable.

        props = app.widget('schematicEditor').properties()
        # assert (props['x'], props['y']) == new_sch_pos
        assert (props['width'], props['height']) == new_sch_size

        props = app.widget('boardEditor').properties()
        # assert (props['x'], props['y']) == new_brd_pos
        assert (props['width'], props['height']) == new_brd_size

        # hide all toolbars and docks
        app.widget('schematicEditorToolbarFile').set_properties(visible=False)
        app.widget('schematicEditorDockErcMsg').set_properties(visible=False)
        app.widget('boardEditorToolbarFile').set_properties(visible=False)
        app.widget('boardEditorDockErcMsg').set_properties(visible=False)

        # close application with specified method
        close_method(app)

    # restart librepcb
    with librepcb.open() as app:
        # check if all toolbars and docks are hidden
        assert app.widget('schematicEditorToolbarFile', wait_active=False).properties()['visible'] is False
        assert app.widget('schematicEditorDockErcMsg', wait_active=False).properties()['visible'] is False
        assert app.widget('boardEditorToolbarFile', wait_active=False).properties()['visible'] is False
        assert app.widget('boardEditorDockErcMsg', wait_active=False).properties()['visible'] is False
