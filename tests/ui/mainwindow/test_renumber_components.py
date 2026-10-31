#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the renumber components panel
"""

from slint_testing import keys


def _rename_component_in_circuit(librepcb, old_name, new_name):
    """
    Rename a component in the test project (before opening it)
    """
    path = librepcb.abspath("Gerber Test/circuit/circuit.lp")
    with open(path, "r") as f:
        content = f.read()
    assert content.count(f'(name "{old_name}")') == 1
    with open(path, "w") as f:
        f.write(content.replace(f'(name "{old_name}")', f'(name "{new_name}")'))


def _open_panel(app):
    app.get("MainMenuBar::project-menu-item").click()
    app.get("MainMenuBar::renumber-components-item").click()
    app.get("RenumberComponentsPanel::header").wait_for(
        label="Renumber Components".upper()
    )


def test_apply(librepcb):
    librepcb.add_project("Gerber Test")
    librepcb.set_project("Gerber Test/project.lpp")
    _rename_component_in_circuit(librepcb, "D1", "D10")
    with librepcb.open() as app:
        # Sidebar button is only shown while the panel is open
        sidebar_btn = app.get("SideBar::renumber-components-btn ?")
        sidebar_btn.wait(0)

        # Open the panel
        _open_panel(app)
        sidebar_btn.wait(1)
        sidebar_btn.wait_for(checked=True)

        # Verify that exactly this rename is proposed (the only component
        # with an unnatural designator in this project)
        texts = app.get("RenumberComponentsPanel::rename-txt *")
        assert texts.label == ["D10 → D1"]

        # Apply the renames
        app.get("RenumberComponentsPanel::apply-btn").click()

        # Verify that there is nothing to renumber anymore
        app.get("RenumberComponentsPanel::view ?").wait(0)
        app.get("RenumberComponentsPanel::apply-btn ?").wait(0)
        app.get("RenumberComponentsPanel::success-txt").wait_for(
            label="✓ All components are numbered nicely."
        )

        # Close the panel with the 'X' button
        app.get("RenumberComponentsPanel::close-btn").click()
        app.get("#RenumberComponentsPanel ?").wait(0)
        app.get("#DocumentsPanel")
        sidebar_btn.wait(0)


def test_nothing_to_renumber(librepcb):
    librepcb.add_project("Gerber Test")
    librepcb.set_project("Gerber Test/project.lpp")
    with librepcb.open() as app:
        # Sidebar button is only shown while the panel is open
        sidebar_btn = app.get("SideBar::renumber-components-btn ?")
        sidebar_btn.wait(0)

        # Open the panel
        _open_panel(app)
        sidebar_btn.wait(1)
        sidebar_btn.wait_for(checked=True)

        # Verify empty state (no list, no apply button, but a success text)
        app.get("RenumberComponentsPanel::view ?").wait(0)
        app.get("RenumberComponentsPanel::apply-btn ?").wait(0)
        app.get("RenumberComponentsPanel::success-txt").wait_for(
            label="✓ All components are numbered nicely."
        )

        # Close the panel with the Escape key
        app.trigger_shortcut([keys.Escape])
        app.get("#RenumberComponentsPanel ?").wait(0)
        app.get("#DocumentsPanel")
        sidebar_btn.wait(0)
