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
    assert le.widget('libraryEditorOverviewNameEdit').properties()['text'] == 'Empty Library'
    assert le.widget('libraryEditorOverviewDescriptionEdit').properties()['plainText'] == ''
    assert le.widget('libraryEditorOverviewKeywordsEdit').properties()['text'] == ''
    assert le.widget('libraryEditorOverviewAuthorEdit').properties()['text'] == 'test'
    assert le.widget('libraryEditorOverviewVersionEdit').properties()['text'] == '0.1'
