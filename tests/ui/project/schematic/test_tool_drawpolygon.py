"""
Test the schematic editor "draw polygon" tool
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
        tabs.wait_for_item(lambda x: x.label == "Schematic").click()
        tab = app.get("#SchematicTab")
        tab.get("SchematicTab::tool-polygon-btn").click()
        toolbar = tab.get("SchematicTab::polygon-toolbar")
        return (
            toolbar.get("DrawPolygonToolBar::layer-cbx"),
            toolbar.get("DrawPolygonToolBar::line-width-edt"),
            toolbar.get("DrawPolygonToolBar::angle-edt"),
            toolbar.get("DrawPolygonToolBar::fill-sw #IconButton"),
        )

    def _select_layer(layer_cbx, layer_name):
        layer_cbx.click()
        layers = layer_cbx.get("DropDownPopup::item-ta *")
        layers.wait_for_item(lambda x: x.label == layer_name).click()

    # Verify defaults and customize all settings.
    with librepcb.open() as app:
        layer_cbx, width_edt, angle_edt, fill_btn = _open_tool(app)

        # Verify default settings.
        layer_cbx.wait_for(value="Guide")  # Anything reasonable
        width_edt.wait_for(value="0.3 millimeters")  # Default
        angle_edt.wait_for(value="0.0°")  # Straight line
        fill_btn.wait_for(checked=False)  # Default

        # Test keyboard shortcuts.
        # Layer
        app.trigger_shortcut([keys.PageDown])
        layer_cbx.wait_for(value="Outlines")
        app.trigger_shortcut([keys.PageUp])
        layer_cbx.wait_for(value="Guide")
        # Line width
        app.trigger_shortcut(["+"])
        width_edt.wait_for(value="0.4 millimeters")
        app.trigger_shortcut(["-"])
        width_edt.wait_for(value="0.3 millimeters")
        # Fill
        app.trigger_shortcut(["f"])
        fill_btn.wait_for(checked=True)

        # Test keyboard navigation.
        # Layer
        app.trigger_shortcut([keys.Tab, keys.DownArrow])
        layer_cbx.wait_for(value="Outlines")
        app.trigger_shortcut([keys.UpArrow])
        layer_cbx.wait_for(value="Guide")
        # Line width
        app.trigger_shortcut([keys.Tab, "50mil", keys.Return])
        width_edt.wait_for(value="50.0 mils")
        app.trigger_shortcut([keys.DownArrow])
        width_edt.wait_for(value="40.0 mils")
        app.trigger_shortcut([keys.UpArrow])
        width_edt.wait_for(value="50.0 mils")
        # Angle
        app.trigger_shortcut([keys.Tab, "90", keys.Return])
        angle_edt.wait_for(value="90.0°")
        app.trigger_shortcut([keys.UpArrow])
        angle_edt.wait_for(value="135.0°")
        app.trigger_shortcut([keys.DownArrow])
        angle_edt.wait_for(value="90.0°")
        # Fill
        app.trigger_shortcut([keys.Tab, keys.Space])
        fill_btn.wait_for(checked=False)

        # Test mouse navigation.
        # Layer
        layer_cbx.click()
        layers = layer_cbx.get("DropDownPopup::item-ta *")
        layers.wait(3, None)
        layers[0].click()
        layer_cbx.wait_for(value="Sheet Frames")
        # Line width
        width_edt.get("LengthEdit::up-btn").click()
        width_edt.wait_for(value="60.0 mils")
        width_edt.get("LengthEdit::down-btn").click()
        width_edt.wait_for(value="50.0 mils")
        # Angle
        angle_edt.get("AngleEdit::up-btn").click()
        angle_edt.wait_for(value="135.0°")
        angle_edt.get("AngleEdit::down-btn").click()
        angle_edt.wait_for(value="90.0°")
        # Fill
        fill_btn.click()
        fill_btn.wait_for(checked=True)

        # For testing properties override of specific layers, also override the
        # properties of a graphical layer.
        _select_layer(layer_cbx, "Documentation")
        width_edt.set_value("80 mils")
        width_edt.get("LengthEdit::up-btn").click()  # Trigger backend sync
        width_edt.wait_for(value="90.0 mils")  # Wait for backend sync

    # Close & reopen the application.
    with librepcb.open() as app:
        layer_cbx, width_edt, angle_edt, fill_btn = _open_tool(app)

        # Test persistence.
        # All properties shall be restored, except the angle which shall be
        # reset to defaults.
        layer_cbx.wait_for(value="Documentation")  # Restored
        width_edt.wait_for(value="90.0 mils")  # Restored
        angle_edt.wait_for(value="0.0°")  # Default
        fill_btn.wait_for(checked=True)  # Restored

        # When switching to another layer, which was not overridden yet, the
        # properties shall be kept.
        _select_layer(layer_cbx, "Values")
        width_edt.wait_for(value="90.0 mils")  # Not changed
        fill_btn.wait_for(checked=True)  # Not changed

        # But when switching to another layer, which has overridden properties,
        # they shall be restored.
        _select_layer(layer_cbx, "Guide")
        width_edt.wait_for(value="50.0 mils")  # Restored
        fill_btn.wait_for(checked=False)  # Restored

        # Test reset menus.
        # Note: The odd numbers in the imperial system are really ugly, but
        # so far we don't have a better idea how to handle this.
        # Line width
        width_edt.trigger()  # Opens the menu
        app.get("DrawPolygonToolBar::line-width-reset-item").click()
        width_edt.wait_for(value="11.81 mils")  # Value reset, but unit kept
