#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the board editor "draw polygon" tool
"""

import time


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


def expected_width(layer):
    if ("Copper" in layer) or (layer in ZERO_WIDTH_LAYERS):
        return "0.0 millimeters"
    return "0.2 millimeters"


def expected_filled(layer):
    return ("Copper" in layer) or (layer in FILLED_LAYERS)


def open_polygon_tool(app):
    """
    Open the board tab and activate the polygon tool. Returns the tool bar
    elements (layer combobox, line width edit, fill toggle).
    """
    tabs = app.get("#TabButton *")
    tabs.wait_for_item(lambda x: x.label == "Board").click()
    tab = app.get("#Board2dTab")
    tab.get("Board2dTab::tool-polygon-btn").click()
    toolbar = tab.get("Board2dTab::polygon-toolbar")
    toolbar.wait(1)
    return (
        toolbar.get("DrawPolygonToolBar::layer-cbx"),
        toolbar.get("DrawPolygonToolBar::line-width-edt"),
        toolbar.get("DrawPolygonToolBar::fill-sw #IconButton"),
    )


def wait_for_width(edt, expected, timeout=2.0):
    """
    Wait until the line width edit shows the expected value. Note that a
    *delayed* update (i.e. only happening on the next user interaction) is
    considered a failure since it would not happen within the timeout.
    """
    start = time.time()
    while True:
        value = edt[0].value
        if value == expected:
            return
        if time.time() - start > timeout:
            raise AssertionError(f"Expected line width {expected!r}, got {value!r}")
        time.sleep(0.05)


def wait_for_width_change(edt, old, timeout=2.0):
    """
    Wait until the line width edit shows any value other than `old`, and
    return the new value.
    """
    start = time.time()
    while True:
        value = edt[0].value
        if value != old:
            return value
        if time.time() - start > timeout:
            raise AssertionError(f"Line width did not change from {old!r}")
        time.sleep(0.05)


def test_layer_switch_restores_defaults(librepcb):
    """
    When switching layers, the line width and "filled" properties shall be
    updated immediately to the newly selected layer's default values, unless
    the defaults are equal (then the values are kept).
    """
    librepcb.add_project("Empty Project")
    librepcb.set_project("Empty Project/Empty Project.lpp")
    with librepcb.open() as app:
        cbx, edt, fill = open_polygon_tool(app)

        # Walk through all layers and verify that the settings of each layer
        # are restored immediately (nothing is stored in a fresh workspace,
        # so the per-layer defaults are expected).
        checked_layers = []
        while True:
            layer = cbx[0].value
            checked_layers.append(layer)
            wait_for_width(edt, expected_width(layer))
            if fill[0].checked != expected_filled(layer):
                raise AssertionError(
                    f"Unexpected 'filled' state on layer {layer!r}: {fill[0].checked}"
                )
            cbx[0]._element.invoke_accessible_increment_action()
            if cbx[0].value == layer:
                break  # Last layer reached

        # Walk backwards again to verify that previously visited layers are
        # still restored correctly.
        for layer in reversed(checked_layers[:-1]):
            cbx[0]._element.invoke_accessible_decrement_action()
            wait_for_width(edt, expected_width(layer))


def test_layer_switch_keeps_customized_width(librepcb):
    """
    A customized line width shall be kept when switching to a layer with the
    same default width, replaced by the default when switching to a layer
    with a different default width, and restored from the client settings
    when switching back to the layer where it was customized.
    """
    librepcb.add_project("Empty Project")
    librepcb.set_project("Empty Project/Empty Project.lpp")
    with librepcb.open() as app:
        cbx, edt, _ = open_polygon_tool(app)

        # Walk to the Documentation layer (default 0.2mm).
        for _ in range(5):
            cbx[0]._element.invoke_accessible_increment_action()
        wait_for_width(edt, expected_width("Documentation"))

        # Customize the line width (this stores it in the client settings).
        tab = app.get("#Board2dTab")
        toolbar = tab.get("Board2dTab::polygon-toolbar")
        up = toolbar.get("DrawPolygonToolBar::line-width-edt LengthEdit::up-btn")
        up.wait(1)
        up[0].click()
        customized = wait_for_width_change(edt, expected_width("Documentation"))

        # Walk forwards through all remaining layers: The customized width
        # must be kept on layers with the same default, and replaced by the
        # default on layers with a different default.
        expected = customized
        previous = "Documentation"
        while True:
            layer = cbx[0].value
            cbx[0]._element.invoke_accessible_increment_action()
            if cbx[0].value == layer:
                break  # Last layer reached
            if cbx[0].value != previous:
                previous = cbx[0].value
                if expected_width(previous) != expected_width(layer):
                    expected = expected_width(previous)
                wait_for_width(edt, expected)

        # Walk back to the Documentation layer and verify that the
        # customized (stored) width is restored from the client settings.
        while cbx[0].value != "Documentation":
            cbx[0]._element.invoke_accessible_decrement_action()
        wait_for_width(edt, customized)
