#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the overview tab in the library editor
"""


def list_dclick_helper(le, widget_name, number):
    # Double click on the list must do nothing
    widget = le.widget(widget_name)
    widget.dclick()
    assert le.widget('libraryEditorStackedWidget').properties()['count'] == number
    assert le.widget('libraryEditorStackedWidget').properties()['currentIndex'] == 0

    # Double click on an item must open the element in a new tab
    widget.dclick_item(widget.model().items().items[0])
    assert le.widget('libraryEditorStackedWidget').properties()['count'] == number + 1
    assert le.widget('libraryEditorStackedWidget').properties()['currentIndex'] == number

    # Switch back to overview tab
    le.widget('libraryEditorTabBar').set_current_tab(0)


def test_edit_with_doubleclick(library_editor, helpers):
    """
    Open editor tabs with doubleclick in the elements list
    """
    list_dclick_helper(library_editor, 'libraryEditorOverviewCmpCatList', 1)
    list_dclick_helper(library_editor, 'libraryEditorOverviewPkgCatList', 2)
    list_dclick_helper(library_editor, 'libraryEditorOverviewSymList', 3)
    list_dclick_helper(library_editor, 'libraryEditorOverviewPkgList', 4)
    list_dclick_helper(library_editor, 'libraryEditorOverviewCmpList', 5)
    list_dclick_helper(library_editor, 'libraryEditorOverviewDevList', 6)


def delete_action_helper(le, widget_name, valid, tabs):
    tabwidget = le.widget('libraryEditorStackedWidget')
    widget = le.widget(widget_name)
    widget.activate_focus()
    le.action('libraryEditorActionRemove').trigger(blocking=False)
    if valid:
        le.widget('libraryEditorOverviewMsgBoxBtnCancel').click()
        tabwidget.wait_for_properties({'count': tabs + 1, 'currentIndex': 0})
    le.action('libraryEditorActionRemove').trigger(blocking=False)
    if valid:
        le.widget('libraryEditorOverviewMsgBoxBtnYes').click()
    tabwidget.wait_for_properties({'count': tabs, 'currentIndex': 0})


def test_delete_action(library_editor, helpers):
    """
    Create removing library elements with the "delete" action
    """
    le = library_editor

    # Open some tabs first
    test_edit_with_doubleclick(library_editor, helpers)
    assert le.widget('libraryEditorStackedWidget').properties()['count'] == 7

    # Press delete while the focus is not in a list widget
    delete_action_helper(le, 'libraryEditorOverviewNameEdit', False, 7)

    # Delete all selected elements
    delete_action_helper(le, 'libraryEditorOverviewCmpCatList', True, 6)
    delete_action_helper(le, 'libraryEditorOverviewPkgCatList', True, 5)
    delete_action_helper(le, 'libraryEditorOverviewSymList', True, 4)
    delete_action_helper(le, 'libraryEditorOverviewPkgList', True, 3)
    delete_action_helper(le, 'libraryEditorOverviewCmpList', True, 2)
    delete_action_helper(le, 'libraryEditorOverviewDevList', True, 1)
