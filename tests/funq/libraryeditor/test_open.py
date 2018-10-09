#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test opening the library editor
"""


def test_metadata(library_editor):
    """
    Just open the library editor and check some metadata
    """
    le = library_editor
    assert le.widget('libraryEditorOverviewNameEdit').properties()['text'] == 'Populated Library'
    assert le.widget('libraryEditorOverviewDescriptionEdit').properties()['plainText'] == 'Test library containing some elements'
    assert le.widget('libraryEditorOverviewKeywordsEdit').properties()['text'] == 'test,library,populated'
    assert le.widget('libraryEditorOverviewAuthorEdit').properties()['text'] == 'test'
    assert le.widget('libraryEditorOverviewVersionEdit').properties()['text'] == '0.0.1'
