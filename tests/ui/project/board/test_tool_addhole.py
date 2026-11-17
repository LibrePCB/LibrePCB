"""
Test the board editor "add hole" tool
"""

import platform

import pytest
from slint_testing import keys


@pytest.mark.skipif(
    platform.system() == "Darwin", reason="LineInput doesn't select-all on focus"
)
def test_toolbar(librepcb):
    """
    Test the toolbar controls by mouse interaction, keyboard navigation and
    keyboard shortcuts. Verifies the default values, persistence, and general
    behavior of the controls.
    """
    librepcb.add_project("Empty Project")
    librepcb.set_project("Empty Project/Empty Project.lpp")

    def _open_tool(app):
        tabs = app.get("#TabButton *")
        tabs.wait_for_item(lambda x: x.label == "Board").click()
        tab = app.get("#Board2dTab")
        tab.get("Board2dTab::tool-hole-btn").click()
        toolbar = tab.get("Board2dTab::hole-toolbar")
        return toolbar.get("AddHoleToolBar::drill-edt")

    # Verify defaults and customize all settings.
    with librepcb.open() as app:
        drill_edt = _open_tool(app)

        # Verify default settings.
        drill_edt.wait_for(value="1.0 millimeters")

        # Test keyboard shortcuts.
        # Drill
        app.trigger_shortcut([keys.Home])
        drill_edt.wait_for(value="1.1 millimeters")
        app.trigger_shortcut([keys.End])
        drill_edt.wait_for(value="1.0 millimeters")

        # Test keyboard navigation.
        # Drill
        app.trigger_shortcut([keys.Tab, "50mil", keys.Return])
        drill_edt.wait_for(value="50.0 mils")
        app.trigger_shortcut([keys.DownArrow])
        drill_edt.wait_for(value="40.0 mils")
        app.trigger_shortcut([keys.UpArrow])
        drill_edt.wait_for(value="50.0 mils")

        # Test mouse navigation.
        # Drill
        drill_edt.get("LengthEdit::up-btn").click()
        drill_edt.wait_for(value="60.0 mils")
        drill_edt.get("LengthEdit::down-btn").click()
        drill_edt.wait_for(value="50.0 mils")

    # Close & reopen the application.
    with librepcb.open() as app:
        drill_edt = _open_tool(app)

        # Test persistence.
        # Drill and unit shall be restored.
        drill_edt.wait_for(value="50.0 mils")
