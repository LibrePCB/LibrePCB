#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pytest

library = 'libraries/Populated Library.lplib'


@pytest.fixture
def project_editor(librepcb, helpers):
    """
    Fixture opening the project editor with an empty project
    """
    librepcb.add_local_library_to_workspace(path=library)
    librepcb.add_project('Empty Project')
    librepcb.set_project('Empty Project/Empty Project.lpp')
    with librepcb.open() as app:
        # Wait for the project to be opened
        helpers.wait_for_project(app, 'Empty Project')

        # Start the actual test
        yield app
