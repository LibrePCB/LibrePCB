"""
Test the board editor "add pad" tool (THT and SMT variants)
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
    librepcb.add_project("Gerber Test")
    librepcb.set_project("Gerber Test/project.lpp")

    def _open_tool(app, menu_item, toolbar):
        tabs = app.get("#TabButton *")
        tabs.wait_for_item(lambda x: x.label == "Board").click()
        tab = app.get("#Board2dTab")
        tab.get("Board2dTab::tool-via-btn").rclick()
        tab.get(f"Board2dTab::{menu_item}").click()
        tb = tab.get(f"Board2dTab::{toolbar}")
        # Collapse sidebar, otherwise the toolbar doesn't fit into the window
        # on the small screen size available on CI.
        app.get("SideBar::documents-btn").set_checked(False)
        return (
            tb.get("AddPadToolBar::net-cbx ?"),
            tb.get("AddPadToolBar::side-selector #IconButton ?"),
            tb.get("AddPadToolBar::shape-selector #ToolBarSelectorButton *"),
            tb.get("AddPadToolBar::width-size-edt"),
            tb.get("AddPadToolBar::height-clearance-edt"),
            tb.get("AddPadToolBar::drill-edt ?"),
            tb.get("AddPadToolBar::ratio-edt ?"),
            tb.get("AddPadToolBar::flags-box #LinkText *"),
        )

    # Verify defaults and customize all settings.
    with librepcb.open() as app:
        ########################################################################
        # THT
        ########################################################################
        (
            net_cbx,
            side_btn,
            shape_btn,
            width_edt,
            height_edt,
            drill_edt,
            radius_edt,
            pressfit_btn,
        ) = _open_tool(app, "tool-pad-tht-item", "pad-tht-toolbar")

        # Verify default settings.
        net_cbx.wait_for(value="[No Net]")
        side_btn.wait_for(valid=False)
        shape_btn.wait_for(checked=[True, False, False, False])  # Round
        width_edt.wait_for(value="2.5 millimeters")
        height_edt.wait_for(value="1.3 millimeters")
        drill_edt.wait_for(value="0.8 millimeters")
        radius_edt.wait_for(value="100.0 %", readonly=True)
        pressfit_btn.wait_for(checked=False)

        # Test keyboard shortcuts.
        # Shape
        app.trigger_shortcut(["3"])
        shape_btn.wait_for(checked=[False, False, True, False])  # Rect
        app.trigger_shortcut(["+"])
        width_edt.wait_for(value="2.6 millimeters")
        app.trigger_shortcut(["-"])
        width_edt.wait_for(value="2.5 millimeters")
        app.trigger_shortcut(["*"])
        height_edt.wait_for(value="1.4 millimeters")
        app.trigger_shortcut(["/"])
        height_edt.wait_for(value="1.3 millimeters")
        app.trigger_shortcut([keys.Home])
        drill_edt.wait_for(value="0.9 millimeters")
        app.trigger_shortcut([keys.End])
        drill_edt.wait_for(value="0.8 millimeters")

        # Test keyboard navigation.
        # Net
        app.trigger_shortcut([keys.Tab, keys.DownArrow])
        net_cbx.wait_for(value="DATA0")
        app.trigger_shortcut([keys.UpArrow])
        net_cbx.wait_for(value="[No Net]")
        # Shape
        app.trigger_shortcut([keys.Tab, keys.RightArrow])
        shape_btn.wait_for(checked=[False, False, False, True])  # Octagon
        app.trigger_shortcut([keys.LeftArrow, keys.LeftArrow])
        shape_btn.wait_for(checked=[False, True, False, False])  # Rounded rect
        # Width
        app.trigger_shortcut([keys.Tab, "2.8", keys.Return])
        width_edt.wait_for(value="2.8 millimeters")
        app.trigger_shortcut([keys.DownArrow])
        width_edt.wait_for(value="2.7 millimeters")
        app.trigger_shortcut([keys.UpArrow])
        width_edt.wait_for(value="2.8 millimeters")
        # Height
        app.trigger_shortcut([keys.Tab, "0.8", keys.Return])
        height_edt.wait_for(value="0.8 millimeters")
        app.trigger_shortcut([keys.DownArrow])
        height_edt.wait_for(value="0.7 millimeters")
        app.trigger_shortcut([keys.UpArrow])
        height_edt.wait_for(value="0.8 millimeters")
        # Drill
        app.trigger_shortcut([keys.Tab, "20mil", keys.Return])
        drill_edt.wait_for(value="20.0 mils")
        app.trigger_shortcut([keys.DownArrow])
        drill_edt.wait_for(value="10.0 mils")
        app.trigger_shortcut([keys.UpArrow])
        drill_edt.wait_for(value="20.0 mils")
        # Radius
        app.trigger_shortcut([keys.Tab, "25", keys.Return])
        radius_edt.wait_for(value="25.0 %")
        app.trigger_shortcut([keys.DownArrow])
        radius_edt.wait_for(value="24.0 %")
        app.trigger_shortcut([keys.UpArrow])
        radius_edt.wait_for(value="25.0 %")
        # Press-Fit
        app.trigger_shortcut([keys.Tab, keys.Space])
        pressfit_btn.wait_for(checked=True)
        app.trigger_shortcut([keys.Space])
        pressfit_btn.wait_for(checked=False)

        # Test mouse navigation.
        # Net
        net_cbx.click()
        nets = net_cbx.get("DropDownPopup::item-ta *")
        nets.wait_for_item(lambda x: x.label == "GND").click()
        net_cbx.wait_for(value="GND")
        # Shape
        shape_btn[0].click()
        shape_btn.wait_for(checked=[True, False, False, False])  # Round
        shape_btn[1].click()
        shape_btn.wait_for(checked=[False, True, False, False])  # Rounded rect
        # Width
        width_edt.get("LengthEdit::up-btn").click()
        width_edt.wait_for(value="2.9 millimeters")
        width_edt.get("LengthEdit::down-btn").click()
        width_edt.wait_for(value="2.8 millimeters")
        # Height
        height_edt.get("LengthEdit::up-btn").click()
        height_edt.wait_for(value="0.9 millimeters")
        height_edt.get("LengthEdit::down-btn").click()
        height_edt.wait_for(value="0.8 millimeters")
        # Drill
        drill_edt.get("LengthEdit::up-btn").click()
        drill_edt.wait_for(value="30.0 mils")
        drill_edt.get("LengthEdit::down-btn").click()
        drill_edt.wait_for(value="20.0 mils")
        # Radius
        radius_edt.get("RatioEdit::up-btn").click()
        radius_edt.wait_for(value="51.0 %")
        radius_edt.get("RatioEdit::down-btn").click()
        radius_edt.wait_for(value="50.0 %")
        # Press-Fit
        pressfit_btn.refresh()  # Not sure why this is needed here...
        pressfit_btn.click()
        pressfit_btn.wait_for(checked=True)
        pressfit_btn.click()
        pressfit_btn.wait_for(checked=False)

        # Switching the shape must automatically update the corner radius.
        width_edt.set_value("0.8mm")
        height_edt.set_value("0.8mm")
        shape_btn[0].click()  # Round
        radius_edt.wait_for(value="100.0 %")
        shape_btn[2].click()  # Rectangular
        radius_edt.wait_for(value="0.0 %")
        shape_btn[1].click()  # Rounded rect
        radius_edt.wait_for(value="50.0 %")
        shape_btn[3].click()  # Octagonal
        radius_edt.wait_for(value="0.0 %")

        # Increasing the width/height in rounded rect mode must automatically
        # reduce the corner radius to the equivalent of 0.5mm.
        shape_btn[1].click()  # Rounded rect
        radius_edt.wait_for(value="50.0 %")
        width_edt.set_value("10mm")
        height_edt.set_value("10mm")
        radius_edt.wait_for(value="5.0 %")

        # Reducing the width or height must automatically decrease the drill.
        width_edt.set_value("0.2mm")
        drill_edt.wait_for(value="7.87 mils")  # 0.2mm
        height_edt.set_value("0.1mm")
        drill_edt.wait_for(value="3.94 mils")  # 0.1mm

        # Increasing the drill must automatically increase width & height.
        drill_edt.set_value("50mils")
        app.trigger_shortcut([keys.Return])
        width_edt.wait_for(value="1.27 millimeters")
        height_edt.wait_for(value="1.27 millimeters")

        # Now set some arbitrary values for testing the persistence.
        shape_btn[3].click()  # Octagonal
        width_edt.set_value("10mm")
        height_edt.set_value("12mm")
        drill_edt.set_value("55mils")
        radius_edt.set_value("42%")
        pressfit_btn.refresh()  # Not sure why this is needed here...
        pressfit_btn.click()
        pressfit_btn.wait_for(checked=True)  # Wait for backend sync

        ########################################################################
        # SMT Standard Pad
        ########################################################################
        (
            net_cbx,
            side_btn,
            shape_btn,
            width_edt,
            height_edt,
            drill_edt,
            radius_edt,
            pressfit_btn,
        ) = _open_tool(app, "tool-pad-smt-item", "pad-smt-toolbar")

        # Verify default settings.
        net_cbx.wait_for(value="[No Net]")
        side_btn.wait_for(checked=False)
        shape_btn.wait_for(checked=[False, True, False, False])  # Rounded rect
        width_edt.wait_for(value="1.5 millimeters")
        height_edt.wait_for(value="0.7 millimeters")
        drill_edt.wait_for(valid=False)
        radius_edt.wait_for(value="50.0 %", readonly=False)
        pressfit_btn.wait_for(valid=False)

        # Test keyboard shortcuts.
        # Component side
        app.trigger_shortcut(["f"])
        side_btn.wait_for(checked=True)

        # Test keyboard navigation.
        # Component side
        app.trigger_shortcut([keys.Tab, keys.Tab, keys.Space])
        side_btn.wait_for(checked=False)

        # Test mouse navigation.
        # Component side
        side_btn.click()
        side_btn.wait_for(checked=True)

        # Now set some arbitrary values for testing the persistence.
        side_btn.set_checked(True)
        shape_btn[2].click()  # Rectangular
        width_edt.set_value("11mm")
        height_edt.set_value("13mm")
        height_edt.wait_for(value="13.0 millimeters")  # Wait for backend sync

        ########################################################################
        # SMT Thermal Pad
        ########################################################################
        (
            net_cbx,
            side_btn,
            shape_btn,
            width_edt,
            height_edt,
            drill_edt,
            radius_edt,
            pressfit_btn,
        ) = _open_tool(app, "tool-pad-thermal-item", "pad-smt-toolbar")

        # Verify default settings.
        net_cbx.wait_for(value="[No Net]")
        side_btn.wait_for(checked=False)
        shape_btn.wait_for(checked=[False, False, True, False])  # Rect
        width_edt.wait_for(value="2.0 millimeters")
        height_edt.wait_for(value="2.0 millimeters")
        drill_edt.wait_for(valid=False)
        radius_edt.wait_for(value="0.0 %", readonly=True)
        pressfit_btn.wait_for(valid=False)

        # Now set some arbitrary values for testing the persistence.
        side_btn.set_checked(False)
        shape_btn[1].click()  # Rounded rect
        width_edt.set_value("12mm")
        height_edt.set_value("14mm")
        radius_edt.set_value("55%")
        radius_edt.wait_for(value="55.0 %")  # Wait for backend sync

        ########################################################################
        # SMT Edge Connector Pad
        ########################################################################
        (
            net_cbx,
            side_btn,
            shape_btn,
            width_edt,
            height_edt,
            drill_edt,
            radius_edt,
            pressfit_btn,
        ) = _open_tool(app, "tool-pad-edge-connector-item", "pad-smt-toolbar")

        # Verify default settings.
        net_cbx.wait_for(value="[No Net]")
        side_btn.wait_for(checked=False)
        shape_btn.wait_for(checked=[False, False, True, False])  # Rect
        width_edt.wait_for(value="1.5 millimeters")
        height_edt.wait_for(value="0.7 millimeters")
        drill_edt.wait_for(valid=False)
        radius_edt.wait_for(value="0.0 %", readonly=True)
        pressfit_btn.wait_for(valid=False)

        ########################################################################
        # SMT Test Pad
        ########################################################################
        (
            net_cbx,
            side_btn,
            shape_btn,
            width_edt,
            height_edt,
            drill_edt,
            radius_edt,
            pressfit_btn,
        ) = _open_tool(app, "tool-pad-test-point-item", "pad-smt-toolbar")

        # Verify default settings.
        net_cbx.wait_for(value="[No Net]")
        side_btn.wait_for(checked=False)
        shape_btn.wait_for(checked=[True, False, False, False])  # Round
        width_edt.wait_for(value="0.7 millimeters")
        height_edt.wait_for(value="0.7 millimeters")
        drill_edt.wait_for(valid=False)
        radius_edt.wait_for(value="100.0 %", readonly=True)
        pressfit_btn.wait_for(valid=False)

        ########################################################################
        # SMT Local Fiducial Pad
        ########################################################################
        (
            net_cbx,
            side_btn,
            shape_btn,
            width_edt,
            height_edt,
            drill_edt,
            radius_edt,
            pressfit_btn,
        ) = _open_tool(app, "tool-pad-local-fiducial-item", "pad-smt-toolbar")

        # Verify default settings.
        net_cbx.wait_for(valid=False)
        side_btn.wait_for(checked=False)
        shape_btn.wait_for(checked=[True, False, False, False])  # Round
        width_edt.wait_for(value="1.0 millimeters")  # Diameter
        height_edt.wait_for(value="0.5 millimeters")  # Copper clearance
        drill_edt.wait_for(valid=False)
        radius_edt.wait_for(value="100.0 %", readonly=True)
        pressfit_btn.wait_for(valid=False)

        # Now set some arbitrary values for testing the persistence.
        height_edt.set_value("5.5mm")  # Copper clearance
        height_edt.wait_for(value="5.5 millimeters")  # Wait for backend sync

        ########################################################################
        # SMT Global Fiducial Pad
        ########################################################################
        (
            net_cbx,
            side_btn,
            shape_btn,
            width_edt,
            height_edt,
            drill_edt,
            radius_edt,
            pressfit_btn,
        ) = _open_tool(app, "tool-pad-global-fiducial-item", "pad-smt-toolbar")

        # Verify default settings.
        net_cbx.wait_for(valid=False)
        side_btn.wait_for(checked=False)
        shape_btn.wait_for(checked=[True, False, False, False])  # Round
        width_edt.wait_for(value="1.0 millimeters")  # Diameter
        height_edt.wait_for(value="0.5 millimeters")  # Copper clearance
        drill_edt.wait_for(valid=False)
        radius_edt.wait_for(value="100.0 %", readonly=True)
        pressfit_btn.wait_for(valid=False)

    # Close & reopen the application.
    with librepcb.open() as app:
        ########################################################################
        # THT
        ########################################################################
        (
            net_cbx,
            side_btn,
            shape_btn,
            width_edt,
            height_edt,
            drill_edt,
            radius_edt,
            pressfit_btn,
        ) = _open_tool(app, "tool-pad-tht-item", "pad-tht-toolbar")

        # Test persistence.
        # All properties shall be restored, except the net and press-fit flag
        # which shall be reset to defaults.
        net_cbx.wait_for(value="[No Net]")  # Default
        shape_btn.wait_for(checked=[False, False, False, True])  # Restored
        width_edt.wait_for(value="10.0 millimeters")  # Restored
        height_edt.wait_for(value="12.0 millimeters")  # Restored
        drill_edt.wait_for(value="55.0 mils")  # Restored
        radius_edt.wait_for(value="42.0 %", readonly=False)  # Restored
        pressfit_btn.wait_for(checked=False)  # Default

        ########################################################################
        # SMT Standard Pad
        ########################################################################
        (
            net_cbx,
            side_btn,
            shape_btn,
            width_edt,
            height_edt,
            drill_edt,
            radius_edt,
            pressfit_btn,
        ) = _open_tool(app, "tool-pad-smt-item", "pad-smt-toolbar")

        # Test persistence.
        # All properties shall be restored, except the net which shall be reset
        # to defaults.
        net_cbx.wait_for(value="[No Net]")  # Default
        side_btn.wait_for(checked=True)  # Restored
        shape_btn.wait_for(checked=[False, False, True, False])  # Restored
        width_edt.wait_for(value="11.0 millimeters")  # Restored
        height_edt.wait_for(value="13.0 millimeters")  # Restored
        radius_edt.wait_for(value="0.0 %", readonly=True)  # Auto

        ########################################################################
        # SMT Thermal Pad
        ########################################################################
        (
            net_cbx,
            side_btn,
            shape_btn,
            width_edt,
            height_edt,
            drill_edt,
            radius_edt,
            pressfit_btn,
        ) = _open_tool(app, "tool-pad-thermal-item", "pad-smt-toolbar")

        # Test persistence.
        # All properties shall be restored, except the net which shall be reset
        # to defaults.
        net_cbx.wait_for(value="[No Net]")  # Default
        side_btn.wait_for(checked=False)  # Restored
        shape_btn.wait_for(checked=[False, True, False, False])  # Restored
        width_edt.wait_for(value="12.0 millimeters")  # Restored
        height_edt.wait_for(value="14.0 millimeters")  # Restored
        radius_edt.wait_for(value="55.0 %", readonly=False)  # Restored

        ########################################################################
        # SMT Local Fiducial Pad
        ########################################################################
        (
            net_cbx,
            side_btn,
            shape_btn,
            width_edt,
            height_edt,
            drill_edt,
            radius_edt,
            pressfit_btn,
        ) = _open_tool(app, "tool-pad-local-fiducial-item", "pad-smt-toolbar")

        # Test persistence.
        # All properties shall be restored, except the net which shall be reset
        # to defaults.
        height_edt.wait_for(value="5.5 millimeters")  # Restored
