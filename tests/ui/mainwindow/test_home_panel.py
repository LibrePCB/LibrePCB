#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the home panel
"""


def test_workspace_folder_expand_collapse(librepcb):
    """
    Clicking the expand/collapse button of a workspace folder item multiple
    times must not crash the app. This is a regression test for a failed
    assert in the Slint library that occurred when toggling the expand state
    of items in the WorkspaceFolderTreeView.
    """
    librepcb.add_project_to_workspace("Empty Project")
    with librepcb.open() as app:
        # Get the expand/collapse buttons (one per item that has children).
        buttons = app.get("#HomePanel #WorkspaceFolderTreeView TreeView::expand-ta *")
        buttons.wait(1, max=None)

        # Click the expand/collapse button several times; This used to crash
        # the app with a failed assert inside the Slint library.
        for _ in range(10):
            buttons[0].click()
