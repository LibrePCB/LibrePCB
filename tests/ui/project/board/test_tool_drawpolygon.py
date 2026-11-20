"""
Test the board editor "draw polygon" tool
"""

import platform

import pytest
from slint_testing import keys

ZERO_WIDTH_LAYERS = [
    "Board Outlines",
    "Board Cutouts",
    "Plated Board Cutouts",
    "Measures",
    "Alignment",
    "Top Glue",
    "Bottom Glue",
    "Top Solder Paste",
    "Bottom Solder Paste",
    "Top Stop Mask",
    "Bottom Stop Mask",
]

FILLED_LAYERS = [
    "Top Glue",
    "Bottom Glue",
    "Top Solder Paste",
    "Bottom Solder Paste",
    "Top Stop Mask",
    "Bottom Stop Mask",
]


def _get_expected_width(layer):
    if ("Copper" in layer) or (layer in ZERO_WIDTH_LAYERS):
        return "0.0 millimeters"
    return "0.2 millimeters"


def _get_expected_filled(layer):
    return ("Copper" in layer) or (layer in FILLED_LAYERS)


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
        tab.get("Board2dTab::tool-polygon-btn").click()
        toolbar = tab.get("Board2dTab::polygon-toolbar")
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
        layer_cbx.wait_for(value="Board Outlines")  # Most important layer
        width_edt.wait_for(value="0.0 millimeters")  # Matching the layer
        angle_edt.wait_for(value="0.0°")  # Straight line
        fill_btn.wait_for(checked=False)  # Matching the layer

        # Walk through all layers and verify that the settings of each layer
        # are restored immediately (nothing is stored in a fresh workspace,
        # so the per-layer defaults are expected).
        layers = []
        while layer_cbx.item_index != 0:
            layer_cbx.decrement()
        while layer_cbx.value not in layers:
            layers.append(layer_cbx.value)
            width_edt.wait_for(value=_get_expected_width(layer_cbx.value))
            fill_btn.wait_for(checked=_get_expected_filled(layer_cbx.value))
            layer_cbx.increment()
        assert len(set(layers)) > len(ZERO_WIDTH_LAYERS)  # Sanity check

        # Walk backwards again to verify that previously visited layers are
        # still restored correctly.
        for layer in reversed(layers[:-1]):
            layer_cbx.decrement()
            width_edt.wait_for(value=_get_expected_width(layer))
            fill_btn.wait_for(checked=_get_expected_filled(layer))

        # Test keyboard shortcuts.
        # Layer
        app.trigger_shortcut([keys.PageDown])
        layer_cbx.wait_for(value="Board Cutouts")
        app.trigger_shortcut([keys.PageUp])
        layer_cbx.wait_for(value="Board Outlines")
        # Line width
        app.trigger_shortcut(["+"])
        width_edt.wait_for(value="0.1 millimeters")
        app.trigger_shortcut(["-"])
        width_edt.wait_for(value="0.09 millimeters")  # 0.0 would be nicer (?)
        # Fill
        app.trigger_shortcut(["f"])
        fill_btn.wait_for(checked=True)

        # Test keyboard navigation.
        # Layer
        app.trigger_shortcut([keys.Tab, keys.DownArrow])
        layer_cbx.wait_for(value="Board Cutouts")
        app.trigger_shortcut([keys.UpArrow])
        layer_cbx.wait_for(value="Board Outlines")
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
        layers[2].click()
        layer_cbx.wait_for(value="Board Cutouts")
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

        # When switching to another layer which has the same default properties,
        # and are not overridden yet, they shall be kept.
        _select_layer(layer_cbx, "Top Documentation")
        width_edt.wait_for(value="90.0 mils")  # Not changed
        fill_btn.wait_for(checked=True)  # Not changed

        # But when changing to another layer which has different default
        # properties, and are not overridden yet, they shall be reset.
        _select_layer(layer_cbx, "Top Stop Mask")
        width_edt.wait_for(value="0.0 mils")  # Reset
        fill_btn.wait_for(checked=True)  # Reset

        # When switching to another layer, which has overridden properties,
        # they shall be restored.
        _select_layer(layer_cbx, "Board Outlines")
        width_edt.wait_for(value="50.0 mils")  # Restored
        fill_btn.wait_for(checked=False)  # Restored

        # Test reset menus.
        # Line width
        width_edt.trigger()  # Opens the menu
        app.get("DrawPolygonToolBar::line-width-reset-item").click()
        width_edt.wait_for(value="0.0 mils")  # Value reset, but unit kept
