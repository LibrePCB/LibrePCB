"""
Test the board editor "draw zone" tool
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
        tab.get("Board2dTab::tool-zone-btn").click()
        toolbar = tab.get("Board2dTab::zone-toolbar")
        return (
            toolbar.get("DrawZoneToolBar::layer-cbx"),
            toolbar.get("DrawZoneToolBar::rules #LinkText *"),
            toolbar.get("DrawZoneToolBar::angle-edt"),
        )

    # Verify defaults and customize all settings.
    with librepcb.open() as app:
        layer_cbx, flags_btn, angle_edt = _open_tool(app)

        # Verify default settings.
        layer_cbx.wait_for(value="Top Copper")
        flags_btn.wait_for(checked=[True, True, True, True])
        angle_edt.wait_for(value="0.0°")

        # Test keyboard shortcuts.
        # Layer
        app.trigger_shortcut([keys.PageDown])
        layer_cbx.wait_for(value="Bottom Copper")
        app.trigger_shortcut([keys.PageUp])
        layer_cbx.wait_for(value="Top Copper")

        # Test keyboard navigation.
        # Layer
        app.trigger_shortcut([keys.Tab, keys.DownArrow])
        layer_cbx.wait_for(value="Bottom Copper")
        app.trigger_shortcut([keys.UpArrow])
        layer_cbx.wait_for(value="Top Copper")
        # Rules
        app.trigger_shortcut([keys.Tab, keys.Space])
        flags_btn.wait_for(checked=[False, True, True, True])
        app.trigger_shortcut([keys.RightArrow, keys.Space])
        flags_btn.wait_for(checked=[False, False, True, True])
        # Angle
        app.trigger_shortcut([keys.Tab, "90", keys.Return])
        angle_edt.wait_for(value="90.0°")
        app.trigger_shortcut([keys.UpArrow])
        angle_edt.wait_for(value="135.0°")
        app.trigger_shortcut([keys.DownArrow])
        angle_edt.wait_for(value="90.0°")

        # Test mouse navigation.
        # Layer
        layer_cbx.click()
        layers = layer_cbx.get("DropDownPopup::item-ta *")
        layers.wait_for(label=["Top Copper", "Bottom Copper"])
        layers[1].click()
        layer_cbx.wait_for(value="Bottom Copper")
        # Rules
        flags_btn.refresh()
        flags_btn[2].click()
        flags_btn.wait_for(checked=[False, False, False, True])
        # Angle
        angle_edt.get("AngleEdit::up-btn").click()
        angle_edt.wait_for(value="135.0°")
        angle_edt.get("AngleEdit::down-btn").click()
        angle_edt.wait_for(value="90.0°")

        # Sanity check that none of the values is back at its default
        layer_cbx.wait_for(value="Bottom Copper")
        flags_btn.wait_for(checked=[False, False, False, True])
        angle_edt.wait_for(value="90.0°")

    # Close & reopen the application.
    with librepcb.open() as app:
        layer_cbx, flags_btn, angle_edt = _open_tool(app)

        # Test persistence.
        # Rules shall be restored, layer and angle shall be reset to defaults.
        layer_cbx.wait_for(value="Top Copper")
        flags_btn.wait_for(checked=[False, False, False, True])
        angle_edt.wait_for(value="0.0°")
