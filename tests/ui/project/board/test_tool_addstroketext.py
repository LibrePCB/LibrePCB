"""
Test the board editor "add stroke text" tool
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
        tab.get("Board2dTab::tool-text-btn").click()
        toolbar = tab.get("Board2dTab::text-toolbar")
        return (
            toolbar.get("AddStrokeTextToolBar::layer-cbx"),
            toolbar.get("AddStrokeTextToolBar::text-edt"),
            toolbar.get("AddStrokeTextToolBar::height-edt"),
            toolbar.get("AddStrokeTextToolBar::stroke-width-edt"),
            toolbar.get("AddStrokeTextToolBar::mirror-sw #IconButton"),
        )

    # Verify defaults and customize all settings.
    with librepcb.open() as app:
        layer_cbx, text_edt, height_edt, stroke_edt, mirror_btn = _open_tool(app)

        # Verify default settings.
        layer_cbx.wait_for(value="Top Legend")  # Most useful layer
        text_edt.wait_for(value="{{PROJECT}}")  # No strong opinion on this
        height_edt.wait_for(value="1.5 millimeters")  # Anything reasonable
        stroke_edt.wait_for(value="0.2 millimeters")  # Anything reasonable
        mirror_btn.wait_for(checked=False)  # Matching the top layer

        # Test keyboard shortcuts.
        # Layer
        app.trigger_shortcut([keys.PageDown])
        layer_cbx.wait_for(value="Top Documentation")
        app.trigger_shortcut([keys.PageUp])
        layer_cbx.wait_for(value="Top Legend")
        # Height
        app.trigger_shortcut(["*"])
        height_edt.wait_for(value="2.0 millimeters")
        app.trigger_shortcut(["/"])
        height_edt.wait_for(value="1.5 millimeters")
        # Flip
        app.trigger_shortcut(["f"])
        layer_cbx.wait_for(value="Bottom Legend")
        mirror_btn.wait_for(checked=True)

        # Test keyboard navigation.
        # Layer
        app.trigger_shortcut([keys.Tab, keys.DownArrow])
        layer_cbx.wait_for(value="Bottom Documentation")
        app.trigger_shortcut([keys.UpArrow])
        layer_cbx.wait_for(value="Bottom Legend")
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
        # Stroke width
        app.trigger_shortcut([keys.Tab, "0.02in", keys.Return])
        stroke_edt.wait_for(value="0.02 inches")
        app.trigger_shortcut([keys.DownArrow])
        stroke_edt.wait_for(value="0.01 inches")
        app.trigger_shortcut([keys.UpArrow])
        stroke_edt.wait_for(value="0.02 inches")
        # Mirror
        app.trigger_shortcut([keys.Tab, keys.Space])
        mirror_btn.wait_for(checked=False)

        # Test mouse navigation.
        # Layer
        layer_cbx.click()
        layers = layer_cbx.get("DropDownPopup::item-ta *")
        layers.wait(2, None)
        layers[1].click()
        layer_cbx.wait_for(value="Board Cutouts")
        # Text
        text_edt.expand()
        texts = text_edt.get("DropDownPopup::item-ta *")
        texts.wait(2, None)
        texts[0].click()
        text_edt.wait_for(value="{{BOARD}}")
        # Height
        height_edt.get("LengthEdit::up-btn").click()
        height_edt.wait_for(value="60.0 mils")
        height_edt.get("LengthEdit::down-btn").click()
        height_edt.wait_for(value="50.0 mils")
        # Stroke width
        stroke_edt.get("LengthEdit::up-btn").click()
        stroke_edt.wait_for(value="0.03 inches")
        stroke_edt.get("LengthEdit::down-btn").click()
        stroke_edt.wait_for(value="0.02 inches")
        # Mirror
        mirror_btn.click()
        mirror_btn.wait_for(checked=True)

    # Close & reopen the application.
    with librepcb.open() as app:
        layer_cbx, text_edt, height_edt, stroke_edt, mirror_btn = _open_tool(app)

        # Test persistence.
        # All properties shall be restored, except the text which shall be
        # reset to defaults.
        layer_cbx.wait_for(value="Board Cutouts")  # Restored
        text_edt.wait_for(value="{{PROJECT}}")  # Default
        height_edt.wait_for(value="50.0 mils")  # Restored
        stroke_edt.wait_for(value="0.02 inches")  # Restored
        mirror_btn.wait_for(checked=True)  # Restored

        # Test reset menus.
        # Note: The odd numbers in the imperial system are really ugly, but
        # so far we don't have a better idea how to handle this.
        # Height
        height_edt.trigger()  # Opens the menu
        app.get("AddStrokeTextToolBar::height-reset-item").click()
        height_edt.wait_for(value="59.06 mils")  # Value reset, but unit kept
        # Stroke width
        stroke_edt.trigger()  # Opens the menu
        app.get("AddStrokeTextToolBar::stroke-width-reset-item").click()
        stroke_edt.wait_for(value="0.00787 inches")  # Value reset, but unit kept
