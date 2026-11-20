"""
Test the board editor "add via" tool
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

    Note: The code for testing via drill & size is identical to
          test_tool_drawtrace.py, keep them in sync!
    """
    librepcb.add_project("Gerber Test")
    librepcb.set_project("Gerber Test/project.lpp")

    def _open_tool(app):
        tabs = app.get("#TabButton *")
        tabs.wait_for_item(lambda x: x.label == "Board").click()
        tab = app.get("#Board2dTab")
        tab.get("Board2dTab::tool-via-btn").click()
        toolbar = tab.get("Board2dTab::via-toolbar")
        return (
            toolbar.get("AddViaToolBar::net-cbx"),
            toolbar.get("AddViaToolBar::via-drill-edt LengthEdit::auto-btn"),
            toolbar.get("AddViaToolBar::via-drill-edt"),
            toolbar.get("AddViaToolBar::via-size-edt LengthEdit::auto-btn"),
            toolbar.get("AddViaToolBar::via-size-edt"),
        )

    # Verify defaults and customize all settings.
    with librepcb.open() as app:
        net_cbx, drill_btn, drill_edt, size_btn, size_edt = _open_tool(app)

        # Verify default settings.
        net_cbx.wait_for(value="[Auto Net]")
        drill_btn.wait_for(checked=True)  # Auto
        drill_edt.wait_for(value="0.3 millimeters")
        size_btn.wait_for(checked=True)  # Auto
        size_edt.wait_for(value="0.7 millimeters")

        # Test keyboard shortcuts.
        # Drill
        app.trigger_shortcut([keys.Home])
        drill_edt.wait_for(value="0.4 millimeters")
        app.trigger_shortcut([keys.End])
        drill_edt.wait_for(value="0.3 millimeters")
        # Size
        app.trigger_shortcut(["*"])
        size_edt.wait_for(value="0.8 millimeters")
        app.trigger_shortcut(["/"])
        size_edt.wait_for(value="0.7 millimeters")

        # Test keyboard navigation.
        # Net
        app.trigger_shortcut([keys.Tab, keys.DownArrow])
        net_cbx.wait_for(value="[No Net]")
        app.trigger_shortcut([keys.UpArrow])
        net_cbx.wait_for(value="[Auto Net]")
        # Drill
        app.trigger_shortcut([keys.Tab, "50mil", keys.Return])
        drill_edt.wait_for(value="50.0 mils")
        app.trigger_shortcut([keys.DownArrow])
        drill_edt.wait_for(value="40.0 mils")
        app.trigger_shortcut([keys.UpArrow])
        drill_edt.wait_for(value="50.0 mils")
        app.trigger_shortcut(["a"])
        drill_btn.wait_for(checked=True)  # Auto
        # Size
        app.trigger_shortcut([keys.Tab, '0.2"', keys.Return])
        size_edt.wait_for(value="0.2 inches")
        app.trigger_shortcut([keys.DownArrow])
        size_edt.wait_for(value="0.1 inches")
        app.trigger_shortcut([keys.UpArrow])
        size_edt.wait_for(value="0.2 inches")
        app.trigger_shortcut(["a"])
        size_btn.wait_for(checked=True)  # Auto

        # Test mouse navigation.
        # Net
        net_cbx.click()
        nets = net_cbx.get("DropDownPopup::item-ta *")
        nets.wait_for_item(lambda x: x.label == "GND").click()
        net_cbx.wait_for(value="GND")
        # Drill
        drill_edt.get("LengthEdit::up-btn").click()
        drill_edt.wait_for(value="15.75 mils")  # 0.3mm + 0.1mm = 0.4mm
        drill_edt.get("LengthEdit::down-btn").click()
        drill_edt.wait_for(value="11.81 mils")  # 0.4mm - 0.1mm = 0.3mm
        drill_btn.click()
        drill_btn.wait_for(checked=True)  # Auto
        # Size
        size_edt.get("LengthEdit::up-btn").click()
        size_edt.wait_for(value="0.0315 inches")  # 0.7mm + 0.1mm = 0.8mm
        size_edt.get("LengthEdit::down-btn").click()
        size_edt.wait_for(value="0.02756 inches")  # 0.8mm - 0.1mm = 0.7mm
        size_btn.click()
        size_btn.wait_for(checked=True)  # Auto

        # Enabling auto drill must automatically enable auto size.
        size_btn.set_checked(False)
        drill_btn.set_checked(False)
        size_btn.wait_for(checked=False)  # Manual
        drill_btn.wait_for(checked=False)  # Manual
        drill_btn.click()
        drill_btn.wait_for(checked=True)  # Auto
        size_btn.wait_for(checked=True)  # Auto

        # Modifying the drill must automatically disable auto drill, but
        # keep auto size. Also the size value must have been updated.
        size_btn.set_checked(True)
        drill_btn.set_checked(True)
        size_btn.wait_for(checked=True)  # Auto
        drill_btn.wait_for(checked=True)  # Auto
        drill_edt.get("LengthEdit::up-btn").click()
        drill_btn.wait_for(checked=False)  # Manual
        size_btn.wait_for(checked=True)  # Auto
        size_edt.wait_for(value="0.0315 inches")  # 0.7mm -> 0.8mm

        # Disabling auto size must automatically disable auto drill.
        size_btn.set_checked(True)
        drill_btn.set_checked(True)
        size_btn.wait_for(checked=True)  # Auto
        drill_btn.wait_for(checked=True)  # Auto
        size_btn.click()
        size_btn.wait_for(checked=False)  # Manual
        drill_btn.wait_for(checked=False)  # Manual

        # Modifying the size must automatically disable auto size and auto
        # drill.
        size_btn.set_checked(True)
        drill_btn.set_checked(True)
        size_btn.wait_for(checked=True)  # Auto
        drill_btn.wait_for(checked=True)  # Auto
        size_edt.get("LengthEdit::up-btn").click()
        drill_btn.wait_for(checked=False)  # Manual
        size_btn.wait_for(checked=False)  # Manual
        size_edt.wait_for(value="0.0315 inches")  # 0.7mm -> 0.8mm

        # Change the drill as well for testing persistence.
        drill_edt.get("LengthEdit::up-btn").click()
        drill_edt.wait_for(value="15.75 mils")  # 0.3mm + 0.1mm = 0.4mm

    # Close & reopen the application.
    # (manual drill, manual size)
    with librepcb.open() as app:
        net_cbx, drill_btn, drill_edt, size_btn, size_edt = _open_tool(app)

        # Test persistence.
        # All properties shall be restored, except the net which shall be
        # reset to defaults.
        net_cbx.wait_for(value="[Auto Net]")  # Default
        drill_btn.wait_for(checked=False)  # Restored
        drill_edt.wait_for(value="15.75 mils")  # Restored
        size_btn.wait_for(checked=False)  # Restored
        size_edt.wait_for(value="0.0315 inches")  # Restored

        # Now use auto size to test its persistence. Increment first, to
        # allow verifying if really the auto mode is restored, not the value.
        size_edt.get("LengthEdit::up-btn").click()
        size_btn.click()
        size_btn.wait_for(checked=True)  # Auto

    # Close & reopen the application.
    # (manual drill, auto size)
    with librepcb.open() as app:
        net_cbx, drill_btn, drill_edt, size_btn, size_edt = _open_tool(app)

        # Test persistence.
        # All properties shall be restored, except the net which shall be
        # reset to defaults.
        net_cbx.wait_for(value="[Auto Net]")  # Default
        drill_btn.wait_for(checked=False)  # Restored
        drill_edt.wait_for(value="15.75 mils")  # Restored
        size_btn.wait_for(checked=True)  # Restored
        size_edt.wait_for(value="0.0315 inches")  # Auto (0.8mm)

        # Now use auto drill to test its persistence.
        drill_btn.click()
        drill_btn.wait_for(checked=True)  # Auto

    # Close & reopen the application.
    # (auto drill, auto size)
    with librepcb.open() as app:
        net_cbx, drill_btn, drill_edt, size_btn, size_edt = _open_tool(app)

        # Test persistence.
        # All properties shall be restored, except the net which shall be
        # reset to defaults.
        net_cbx.wait_for(value="[Auto Net]")  # Default
        drill_btn.wait_for(checked=True)  # Restored
        drill_edt.wait_for(value="11.81 mils")  # Auto (0.3mm)
        size_btn.wait_for(checked=True)  # Restored
        size_edt.wait_for(value="0.02756 inches")  # Auto (0.7mm)
