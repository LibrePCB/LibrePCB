"""
Test the board editor "draw trace" tool
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
          test_tool_addvia.py, keep them in sync!
    """
    librepcb.add_project("Gerber Test")
    librepcb.set_project("Gerber Test/project.lpp")

    def _open_tool(app):
        tabs = app.get("#TabButton *")
        tabs.wait_for_item(lambda x: x.label == "Board").click()
        tab = app.get("#Board2dTab")
        tab.get("Board2dTab::tool-trace-btn").click()
        toolbar = tab.get("Board2dTab::trace-toolbar")
        # Collapse sidebar, otherwise the toolbar doesn't fit into the window
        # on the small screen size available on CI.
        app.get("SideBar::documents-btn").set_checked(False)
        return (
            toolbar.get("DrawTraceToolBar::layer-cbx"),
            toolbar.get("DrawTraceToolBar::line-width-edt LengthEdit::auto-btn"),
            toolbar.get("DrawTraceToolBar::line-width-edt"),
            toolbar.get("DrawTraceToolBar::via-drill-edt LengthEdit::auto-btn"),
            toolbar.get("DrawTraceToolBar::via-drill-edt"),
            toolbar.get("DrawTraceToolBar::via-size-edt LengthEdit::auto-btn"),
            toolbar.get("DrawTraceToolBar::via-size-edt"),
            toolbar.get(
                "DrawTraceToolBar::wire-mode-selector #ToolBarSelectorButton *"
            ),
            toolbar.get("DrawTraceToolBar::clearance-circle-btn #IconButton"),
        )

    # Verify defaults and customize all settings.
    with librepcb.open() as app:
        (
            layer_cbx,
            width_btn,
            width_edt,
            drill_btn,
            drill_edt,
            size_btn,
            size_edt,
            mode_btn,
            clearance_btn,
        ) = _open_tool(app)

        # Verify default settings.
        layer_cbx.wait_for(value="Top Copper")
        width_btn.wait_for(checked=True)  # Auto
        width_edt.wait_for(value="0.5 millimeters")
        drill_btn.wait_for(checked=True)  # Auto
        drill_edt.wait_for(value="0.3 millimeters")
        size_btn.wait_for(checked=True)  # Auto
        size_edt.wait_for(value="0.7 millimeters")
        mode_btn.wait_for(checked=[False, False, True, False, False])  # 90°/45°
        clearance_btn.wait_for(checked=False)  # May be changed some day

        # Test keyboard shortcuts.
        # Layer
        app.trigger_shortcut([keys.PageDown])
        layer_cbx.wait_for(value="Bottom Copper")
        app.trigger_shortcut([keys.PageUp])
        layer_cbx.wait_for(value="Top Copper")
        # Width
        app.trigger_shortcut(["+"])
        width_edt.wait_for(value="0.6 millimeters")
        app.trigger_shortcut(["-"])
        width_edt.wait_for(value="0.5 millimeters")
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
        # Mode
        app.trigger_shortcut(["5"])
        mode_btn.wait_for(checked=[False, False, False, False, True])  # Straight
        app.trigger_shortcut(["3"])
        mode_btn.wait_for(checked=[False, False, True, False, False])  # 90°/45°

        # Test keyboard navigation.
        # Layer
        app.trigger_shortcut([keys.Tab, keys.DownArrow])
        layer_cbx.wait_for(value="Bottom Copper")
        app.trigger_shortcut([keys.UpArrow])
        layer_cbx.wait_for(value="Top Copper")
        # Width
        app.trigger_shortcut([keys.Tab, "800um", keys.Return])
        width_edt.wait_for(value="800.0 micrometers")
        app.trigger_shortcut([keys.DownArrow])
        width_edt.wait_for(value="700.0 micrometers")
        app.trigger_shortcut([keys.UpArrow])
        width_edt.wait_for(value="800.0 micrometers")
        app.trigger_shortcut(["a"])
        width_btn.wait_for(checked=True)  # Auto
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
        # Mode
        app.trigger_shortcut([keys.Tab, keys.LeftArrow])
        mode_btn.wait_for(checked=[False, True, False, False, False])  # V/H
        app.trigger_shortcut([keys.RightArrow])
        mode_btn.wait_for(checked=[False, False, True, False, False])  # 90°/45°
        # Clearance circle
        app.trigger_shortcut([keys.Tab, keys.Space])
        clearance_btn.wait_for(checked=True)

        # Test mouse navigation.
        # Layer
        layer_cbx.click()
        layers = layer_cbx.get("DropDownPopup::item-ta *")
        layers.wait_for(label=["Top Copper", "Bottom Copper"])
        layers[1].click()
        layer_cbx.wait_for(value="Bottom Copper")
        # Width
        width_edt.get("LengthEdit::up-btn").click()
        width_edt.wait_for(value="900.0 micrometers")
        width_edt.get("LengthEdit::down-btn").click()
        width_edt.wait_for(value="800.0 micrometers")
        width_btn.click()
        width_btn.wait_for(checked=True)  # Auto
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
        # Mode
        mode_btn[4].click()
        mode_btn.wait_for(checked=[False, False, False, False, True])  # Straight
        # Clearance circle
        clearance_btn.click()
        clearance_btn.wait_for(checked=False)

        # Modifying the trace width in idle mode shall disable the auto width.
        # Otherwise, the width will be reset when starting a new trace.
        width_btn.set_checked(True)
        width_btn.wait_for(checked=True)  # Auto
        width_edt.get("LengthEdit::up-btn").click()
        width_btn.wait_for(checked=False)  # Important: Must be disabled!
        width_edt.wait_for(value="900.0 micrometers")  # 0.8mm -> 0.9mm

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

        # Change settings for testing persistence.
        clearance_btn.set_checked(True)
        drill_edt.get("LengthEdit::up-btn").click()
        drill_edt.wait_for(value="15.75 mils")  # 0.3mm + 0.1mm = 0.4mm

    # Close & reopen the application.
    # (manual width, manual drill, manual size)
    with librepcb.open() as app:
        (
            layer_cbx,
            width_btn,
            width_edt,
            drill_btn,
            drill_edt,
            size_btn,
            size_edt,
            mode_btn,
            clearance_btn,
        ) = _open_tool(app)

        # Test persistence.
        # All properties shall be restored, except the layer which shall be
        # reset to defaults.
        layer_cbx.wait_for(value="Top Copper")
        width_btn.wait_for(checked=False)  # Auto
        width_edt.wait_for(value="900.0 micrometers")
        drill_btn.wait_for(checked=False)  # Restored
        drill_edt.wait_for(value="15.75 mils")  # Restored
        size_btn.wait_for(checked=False)  # Restored
        size_edt.wait_for(value="0.0315 inches")  # Restored
        mode_btn.wait_for(checked=[False, False, False, False, True])  # Restored
        clearance_btn.wait_for(checked=True)  # Restored

        # Now use auto size to test its persistence. Increment first, to
        # allow verifying if really the auto mode is restored, not the value.
        size_edt.get("LengthEdit::up-btn").click()
        size_btn.click()
        size_btn.wait_for(checked=True)  # Auto

    # Close & reopen the application.
    # (manual width, manual drill, auto size)
    with librepcb.open() as app:
        (
            layer_cbx,
            width_btn,
            width_edt,
            drill_btn,
            drill_edt,
            size_btn,
            size_edt,
            mode_btn,
            clearance_btn,
        ) = _open_tool(app)

        # Test persistence.
        # All properties shall be restored, except the layer which shall be
        # reset to defaults.
        layer_cbx.wait_for(value="Top Copper")
        width_btn.wait_for(checked=False)  # Auto
        width_edt.wait_for(value="900.0 micrometers")
        drill_btn.wait_for(checked=False)  # Restored
        drill_edt.wait_for(value="15.75 mils")  # Restored
        size_btn.wait_for(checked=True)  # Restored
        size_edt.wait_for(value="0.0315 inches")  # Auto (0.8mm)
        mode_btn.wait_for(checked=[False, False, False, False, True])  # Restored
        clearance_btn.wait_for(checked=True)  # Restored

        # Now use auto width & auto drill to test its persistence.
        width_btn.click()
        width_btn.wait_for(checked=True)  # Auto
        drill_btn.click()
        drill_btn.wait_for(checked=True)  # Auto

    # Close & reopen the application.
    # (auto width, auto drill, auto size)
    with librepcb.open() as app:
        (
            layer_cbx,
            width_btn,
            width_edt,
            drill_btn,
            drill_edt,
            size_btn,
            size_edt,
            mode_btn,
            clearance_btn,
        ) = _open_tool(app)

        # Test persistence.
        # All properties shall be restored, except the layer which shall be
        # reset to defaults.
        layer_cbx.wait_for(value="Top Copper")
        width_btn.wait_for(checked=True)  # Restored
        width_edt.wait_for(value="500.0 micrometers")  # Auto (0.5mm)
        drill_btn.wait_for(checked=True)  # Restored
        drill_edt.wait_for(value="11.81 mils")  # Auto (0.3mm)
        size_btn.wait_for(checked=True)  # Restored
        size_edt.wait_for(value="0.02756 inches")  # Auto (0.7mm)
        mode_btn.wait_for(checked=[False, False, False, False, True])  # Restored
        clearance_btn.wait_for(checked=True)  # Restored
