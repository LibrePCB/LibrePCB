"""
Test the schematic editor "add component" tool
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
    librepcb.add_local_library_to_workspace("libraries/Populated Library.lplib")
    librepcb.add_project("Empty Project")
    librepcb.set_project("Empty Project/Empty Project.lpp")

    def _open_tool(app, tool_btn):
        tabs = app.get("#TabButton *")
        tabs.wait_for_item(lambda x: x.label == "Schematic").click()
        tab = app.get("#SchematicTab")
        tab.get(f"SchematicTab::{tool_btn}").click()
        toolbar = tab.get("SchematicTab::component-toolbar")
        return (
            toolbar.get("AddComponentToolBar::value-edt"),
            toolbar.get("AddComponentToolBar::attribute-value-edt ?"),
            toolbar.get("AddComponentToolBar::unit-cbx ?"),
        )

    # Verify defaults and customize all settings.
    with librepcb.open() as app:
        value_edt, attr_edt, unit_cbx = _open_tool(app, "tool-resistor-btn")

        # Verify default settings.
        value_edt.wait_for(value="{{RESISTANCE}}")
        attr_edt.wait_for(value="")
        unit_cbx.wait_for(value="Ω")

        # Test keyboard shortcuts.
        # N/A

        # Test keyboard navigation.
        # Value
        app.trigger_shortcut([keys.Tab, "Foo Bar", keys.Return])
        value_edt.wait_for(value="Foo Bar")
        app.trigger_shortcut([keys.DownArrow])  # Choose value from dropdown
        value_edt.wait_for(value="{{RESISTANCE}}")
        # Attribute value
        app.trigger_shortcut([keys.Tab, "10k", keys.Return])
        attr_edt.wait_for(value="10")
        unit_cbx.wait_for(value="kΩ")
        # Attribute unit
        app.trigger_shortcut([keys.Tab, keys.DownArrow])
        attr_edt.wait_for(value="10")
        unit_cbx.wait_for(value="MΩ")

        # Test mouse navigation.
        # Attribute unit
        unit_cbx.click()
        units = unit_cbx.get("DropDownPopup::item-ta *")
        units.wait_for_item(lambda x: x.label == "μΩ").click()
        unit_cbx.wait_for(value="μΩ")

        # Entering a non-attribute value shall hide the attribute widgets.
        value_edt.set_value("Foo Bar")
        attr_edt.wait_for(valid=False)
        unit_cbx.wait_for(valid=False)
