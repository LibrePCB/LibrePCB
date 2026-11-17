"""
Test the schematic editor "add text" tool
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
        tab.get("SchematicTab::tool-text-btn").click()
        toolbar = tab.get("SchematicTab::text-toolbar")
        return (
            toolbar.get("AddTextToolBar::layer-cbx"),
            toolbar.get("AddTextToolBar::text-edt"),
            toolbar.get("AddTextToolBar::height-edt"),
        )

    # Verify defaults and customize all settings.
    with librepcb.open() as app:
        layer_cbx, text_edt, height_edt = _open_tool(app)

        # Verify default settings.
        layer_cbx.wait_for(value="Comments")  # Anything reasonable
        text_edt.wait_for(value="{{PROJECT}}")  # No strong opinion on this
        height_edt.wait_for(value="2.5 millimeters")  # Anything reasonable

        # Test keyboard shortcuts.
        # Layer
        app.trigger_shortcut([keys.PageDown])
        layer_cbx.wait_for(value="Guide")
        app.trigger_shortcut([keys.PageUp])
        layer_cbx.wait_for(value="Comments")
        # Height
        app.trigger_shortcut(["*"])
        height_edt.wait_for(value="3.0 millimeters")
        app.trigger_shortcut(["/"])
        height_edt.wait_for(value="2.5 millimeters")

        # Test keyboard navigation.
        # Layer
        app.trigger_shortcut([keys.Tab, keys.DownArrow])
        layer_cbx.wait_for(value="Guide")
        app.trigger_shortcut([keys.UpArrow])
        layer_cbx.wait_for(value="Comments")
        # Text
        app.trigger_shortcut([keys.Tab, "Foo Bar", keys.Return])
        text_edt.wait_for(value="Foo Bar")
        # Height
        app.trigger_shortcut([keys.Tab, "50mil", keys.Return])
        height_edt.wait_for(value="50.0 mils")
        app.trigger_shortcut([keys.DownArrow])
        height_edt.wait_for(value="40.0 mils")
        app.trigger_shortcut([keys.UpArrow])
        height_edt.wait_for(value="50.0 mils")

        # Test mouse navigation.
        # Layer
        layer_cbx.click()
        layers = layer_cbx.get("DropDownPopup::item-ta *")
        layers.wait(2, None)
        layers[1].click()
        layer_cbx.wait_for(value="Documentation")
        # Text
        text_edt.expand()
        texts = text_edt.get("DropDownPopup::item-ta *")
        texts.wait(2, None)
        texts[0].click()
        text_edt.wait_for(value="{{SHEET}}")
        # Height
        height_edt.get("LengthEdit::up-btn").click()
        height_edt.wait_for(value="60.0 mils")
        height_edt.get("LengthEdit::down-btn").click()
        height_edt.wait_for(value="50.0 mils")

    # Close & reopen the application.
    with librepcb.open() as app:
        layer_cbx, text_edt, height_edt = _open_tool(app)

        # Test persistence.
        # All properties shall be restored, except the text which shall be
        # reset to defaults.
        layer_cbx.wait_for(value="Documentation")  # Restored
        text_edt.wait_for(value="{{PROJECT}}")  # Default
        height_edt.wait_for(value="50.0 mils")  # Restored

        # Test reset menus.
        # Note: The odd numbers in the imperial system are really ugly, but
        # so far we don't have a better idea how to handle this.
        # Height
        height_edt.trigger()  # Opens the menu
        app.get("AddTextToolBar::height-reset-item").click()
        height_edt.wait_for(value="98.43 mils")  # Value reset, but unit kept
