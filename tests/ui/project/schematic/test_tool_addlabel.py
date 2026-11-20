"""
Test the schematic editor "add label" tool
"""

from slint_testing import keys


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
        tab.get("SchematicTab::tool-label-btn").click()
        toolbar = tab.get("SchematicTab::label-toolbar")
        return (
            toolbar.get("AddLabelToolBar::angle-selector #ToolBarSelectorButton *"),
            toolbar.get("AddLabelToolBar::mirror-btn #IconButton"),
            toolbar.get("AddLabelToolBar::auto-orientation-btn #IconButton"),
        )

    # Verify defaults and customize all settings.
    with librepcb.open() as app:
        rotation_btn, mirror_btn, auto_btn = _open_tool(app)

        # Verify default settings.
        rotation_btn.wait_for(checked=[True, False, False, False])  # 0°
        mirror_btn.wait_for(checked=False)
        auto_btn.wait_for(checked=True)

        # Test keyboard shortcuts.
        # Rotation
        app.trigger_shortcut(["r"])
        rotation_btn.wait_for(checked=[False, True, False, False])  # 90°
        app.trigger_shortcut([keys.Shift, "r"])
        rotation_btn.wait_for(checked=[True, False, False, False])  # 0°
        # Mirror
        app.trigger_shortcut(["m"])
        mirror_btn.wait_for(checked=True)
        app.trigger_shortcut(["m"])
        mirror_btn.wait_for(checked=False)
        # Auto
        app.trigger_shortcut([keys.Space])
        auto_btn.wait_for(checked=True)
        app.trigger_shortcut([keys.Space])
        auto_btn.wait_for(checked=False)

        # Test keyboard navigation.
        # Rotation
        app.trigger_shortcut([keys.Tab, keys.RightArrow])
        rotation_btn.wait_for(checked=[False, True, False, False])  # 90°
        app.trigger_shortcut([keys.LeftArrow])
        rotation_btn.wait_for(checked=[True, False, False, False])  # 0°
        # Mirror
        app.trigger_shortcut([keys.Tab, keys.Space])
        mirror_btn.wait_for(checked=True)
        app.trigger_shortcut([keys.Space])
        mirror_btn.wait_for(checked=False)
        # Auto
        app.trigger_shortcut([keys.Tab, keys.Space])
        auto_btn.wait_for(checked=True)
        app.trigger_shortcut([keys.Space])
        auto_btn.wait_for(checked=False)

        # Test mouse navigation.
        # Rotation
        rotation_btn[3].click()
        rotation_btn.wait_for(checked=[False, False, False, True])  # 270°
        # Mirror
        mirror_btn.click()
        mirror_btn.wait_for(checked=True)
        # Auto
        auto_btn.click()
        auto_btn.wait_for(checked=True)

        # Rotate in auto mode shall disable auto mode but keep the mirror state.
        rotation_btn[2].click()
        rotation_btn.wait_for(checked=[False, False, True, False])  # 180°
        mirror_btn.set_checked(True)
        mirror_btn.wait_for(checked=True)  # Wait for backend sync
        auto_btn.set_checked(True)
        auto_btn.wait_for(checked=True)  # Wait for backend sync
        rotation_btn[3].click()  # Rotate
        rotation_btn.wait_for(checked=[False, False, False, True])  # 270°
        auto_btn.wait_for(checked=False)  # Must be disabled!
        mirror_btn.wait_for(checked=True)  # Must be kept!

        # Mirror in auto mode shall disable auto mode but keep the rotation.
        rotation_btn[2].click()
        rotation_btn.wait_for(checked=[False, False, True, False])  # 180°
        mirror_btn.set_checked(False)
        mirror_btn.wait_for(checked=False)  # Wait for backend sync
        auto_btn.set_checked(True)
        auto_btn.wait_for(checked=True)  # Wait for backend sync
        mirror_btn.click()  # Mirror
        mirror_btn.wait_for(checked=True)  # Wait for backend sync
        auto_btn.wait_for(checked=False)  # Must be disabled!
        rotation_btn.wait_for(checked=[False, False, True, False])  # Must be kept!

        # Disabling auto mode shall restore default settings as this is
        # convenient.
        rotation_btn[2].click()
        rotation_btn.wait_for(checked=[False, False, True, False])  # 180°
        mirror_btn.set_checked(True)
        mirror_btn.wait_for(checked=True)  # Wait for backend sync
        auto_btn.set_checked(True)
        auto_btn.wait_for(checked=True)  # Wait for backend sync
        auto_btn.click()  # Disable auto mode
        auto_btn.wait_for(checked=False)  # Wait for backend sync
        mirror_btn.wait_for(checked=False)  # Reset to default!
        rotation_btn.wait_for(checked=[True, False, False, False])  # Reset to default!

        # Change settings for testing persistence.
        auto_btn.set_checked(False)
        auto_btn.wait_for(checked=False)  # Wait for backend sync
        rotation_btn[2].click()
        rotation_btn.wait_for(checked=[False, False, True, False])  # 180°
        mirror_btn.set_checked(True)
        mirror_btn.wait_for(checked=True)  # Wait for backend sync

    # Close & reopen the application.
    with librepcb.open() as app:
        rotation_btn, mirror_btn, auto_btn = _open_tool(app)

        # Test persistence.
        # All properties shall be restored.
        rotation_btn.wait_for(checked=[False, False, True, False])  # Restored
        mirror_btn.wait_for(checked=True)  # Restored
        auto_btn.wait_for(checked=False)  # Restored

        # Enable auto mode to test persistence again.
        auto_btn.click()
        auto_btn.wait_for(checked=True)

    # Close & reopen the application.
    with librepcb.open() as app:
        rotation_btn, mirror_btn, auto_btn = _open_tool(app)

        # Test persistence.
        # Auto mode shall be restored. The other properties we don't verify
        # since they may be dependent on the cursor position.
        auto_btn.wait_for(checked=True)  # Restored
