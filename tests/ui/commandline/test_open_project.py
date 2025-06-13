#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test opening projects by command line arguments
"""


def test_open_lpp(librepcb, helpers):
    """
    Open *.lpp project by command line argument
    """
    librepcb.add_project('Empty Project')
    librepcb.set_project('Empty Project/Empty Project.lpp')
    with librepcb.open() as app:
        helpers.wait_for_project(app, 'Empty Project')


def test_open_lppz(librepcb, helpers):
    """
    Open *.lppz project by command line argument
    """
    librepcb.add_project('Empty Project', as_lppz=True)
    librepcb.set_project('Empty Project.lppz')
    with librepcb.open() as app:
        helpers.wait_for_project(app, 'Empty Project')
