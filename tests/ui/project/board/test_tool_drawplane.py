"""
Test the board editor "draw plane" tool
"""

from slint_testing import keys


def test_toolbar(librepcb):
    """
    Test the toolbar controls by mouse interaction, keyboard navigation and
    keyboard shortcuts. Verifies the default values, persistence, and general
    behavior of the controls.
    """
    librepcb.add_project("Gerber Test")
    librepcb.set_project("Gerber Test/project.lpp")

    def _open_tool(app):
        tabs = app.get("#TabButton *")
        tabs.wait_for_item(lambda x: x.label == "Board").click()
        tab = app.get("#Board2dTab")
        tab.get("Board2dTab::tool-plane-btn").click()
        toolbar = tab.get("Board2dTab::plane-toolbar")
        return (
            toolbar.get("DrawPlaneToolBar::layer-cbx"),
            toolbar.get("DrawPlaneToolBar::net-cbx"),
            toolbar.get("DrawPlaneToolBar::auto-add-btn"),
        )

    # Verify defaults and customize all settings.
    with librepcb.open() as app:
        layer_cbx, net_cbx, auto_btn = _open_tool(app)

        # Verify default settings.
        layer_cbx.wait_for(value="Top Copper")
        net_cbx.wait_for(value="GND")  # Automatically detected from project

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
        # Net
        app.trigger_shortcut([keys.Tab, keys.DownArrow])
        net_cbx.wait_for(value="N1")
        app.trigger_shortcut([keys.UpArrow])
        net_cbx.wait_for(value="GND")

        # Test mouse navigation.
        # Layer
        layer_cbx.click()
        layers = layer_cbx.get("DropDownPopup::item-ta *")
        layers.wait_for(label=["Top Copper", "Bottom Copper"])
        layers[1].click()
        layer_cbx.wait_for(value="Bottom Copper")
        # Net
        net_cbx.click()
        nets = net_cbx.get("DropDownPopup::item-ta *")
        nets.wait(2, None)
        nets[0].click()
        net_cbx.wait_for(value="[No Net]")

        # Test auto-add. It must switch automatically to the next layer.
        layer_cbx.decrement()
        layer_cbx.wait_for(value="Top Copper")
        auto_btn.click()
        layer_cbx.wait_for(value="Bottom Copper")

    # Close & reopen the application.
    with librepcb.open() as app:
        layer_cbx, net_cbx, _add_btn = _open_tool(app)

        # Test persistence.
        # None of the properties are persisted, so the default values shall be
        # restored.
        layer_cbx.wait_for(value="Top Copper")
        net_cbx.wait_for(value="GND")
