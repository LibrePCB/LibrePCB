#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test opening projects by command line arguments
"""


def test_open_lpp(librepcb, helpers):
    """
    Open *.lpp project by command line argument
    """
    librepcb.add_project("Empty Project")
    librepcb.set_project("Empty Project/Empty Project.lpp")
    with librepcb.open() as app:
        projects = app.get("DocumentsPanel::project-item ProjectSection::header *")
        projects.wait(1)
        assert projects.label == ["Empty Project".upper()]


def test_open_lppz(librepcb, helpers):
    """
    Open *.lppz project by command line argument
    """
    librepcb.add_project("Empty Project", as_lppz=True)
    librepcb.set_project("Empty Project.lppz")
    with librepcb.open() as app:
        projects = app.get("DocumentsPanel::project-item ProjectSection::header *")
        projects.wait(1)
        assert projects.label == ["Empty Project".upper()]
