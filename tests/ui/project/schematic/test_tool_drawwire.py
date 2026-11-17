"""
Test the schematic editor "draw wire" tool
"""

from slint_testing import keys


def test_toolbar(librepcb):
    """
    Test the toolbar controls by mouse interaction, keyboard navigation and
    keyboard shortcuts. Verifies the default values, persistence, and general
    behavior of the controls.

    Note: This test is almost identical to test_tool_drawbus.py, keep them
          in sync!
    """
    librepcb.add_project("Empty Project")
    librepcb.set_project("Empty Project/Empty Project.lpp")

    def _open_tool(app):
        tabs = app.get("#TabButton *")
        tabs.wait_for_item(lambda x: x.label == "Schematic").click()
        tab = app.get("#SchematicTab")
        tab.get("SchematicTab::tool-wire-btn").click()
        toolbar = tab.get("SchematicTab::wire-toolbar")
        return toolbar.get(
            "DrawWireToolBar::wire-mode-selector #ToolBarSelectorButton *"
        )

    # Verify defaults and customize all settings.
    with librepcb.open() as app:
        mode_btn = _open_tool(app)

        # Verify default settings.
        mode_btn.wait_for(checked=[True, False, False, False, False])  # H/V

        # Test keyboard shortcuts.
        # Mode
        app.trigger_shortcut(["5"])
        mode_btn.wait_for(checked=[False, False, False, False, True])  # Straight
        app.trigger_shortcut(["3"])
        mode_btn.wait_for(checked=[False, False, True, False, False])  # 90°/45°

        # Test keyboard navigation.
        # Mode
        app.trigger_shortcut([keys.Tab, keys.LeftArrow])
        mode_btn.wait_for(checked=[False, True, False, False, False])  # V/H
        app.trigger_shortcut([keys.RightArrow])
        mode_btn.wait_for(checked=[False, False, True, False, False])  # 90°/45°

        # Test mouse navigation.
        # Mode
        mode_btn[4].click()
        mode_btn.wait_for(checked=[False, False, False, False, True])  # Straight

    # Close & reopen the application.
    with librepcb.open() as app:
        mode_btn = _open_tool(app)

        # Test persistence.
        # All properties shall be restored.
        mode_btn.wait_for(checked=[False, False, False, False, True])  # Restored
