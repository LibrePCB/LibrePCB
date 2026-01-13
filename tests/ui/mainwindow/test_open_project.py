#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test opening projects
"""


def test_file_tree_dclick(librepcb, helpers):
    librepcb.add_project_to_workspace("Empty Project")

    with librepcb.open() as app:
        # Open project
        items = app.get("#HomePanel #WorkspaceFolderTreeView TreeView::item-ta *")
        items.wait(1)
        items[0].dclick()
        items.wait_for_item(lambda x: x.label == "Empty Project.lpp").dclick()

        # Verify that schematic tab has been opened
        tabs = app.get("#TabButton *")
        tabs.wait(2)
        assert tabs[1].label == "Main"
        assert tabs[1].checked

        # Close project and wait until schematic tab has been closed
        app.get("#DocumentsPanel ProjectSection::close-btn").click()
        tabs.wait(1)
