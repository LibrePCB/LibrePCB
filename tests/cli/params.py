#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import pytest


class Project:
    def __init__(self, dir, path, output_dir, board_count):
        self.dir = dir
        self.path = path
        self.parent_dir = os.path.dirname(path)
        self.output_dir = output_dir
        self.is_lppz = path.endswith('.lppz')
        self.board_count = board_count


EMPTY_PROJECT_LPP = Project(
    'Empty Project',
    'Empty Project/Empty Project.lpp',
    'Empty Project/output/v1',
    1,
)
EMPTY_PROJECT_LPP_PARAM = pytest.param(EMPTY_PROJECT_LPP,
                                       id='EmptyProject.lpp')

EMPTY_PROJECT_LPPZ = Project(
    'Empty Project',
    'Empty Project.lppz',
    'output/v1',
    1,
)
EMPTY_PROJECT_LPPZ_PARAM = pytest.param(EMPTY_PROJECT_LPPZ,
                                        id='EmptyProject.lppz')

PROJECT_WITH_TWO_BOARDS_LPP = Project(
    'Project With Two Boards',
    'Project With Two Boards/Project With Two Boards.lpp',
    'Project With Two Boards/output/v1',
    2,
)
PROJECT_WITH_TWO_BOARDS_LPP_PARAM = pytest.param(PROJECT_WITH_TWO_BOARDS_LPP,
                                                 id='ProjectWithTwoBoards.lpp')

PROJECT_WITH_TWO_BOARDS_LPPZ = Project(
    'Project With Two Boards',
    'Project With Two Boards.lppz',
    'output/v1',
    2,
)
PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM = pytest.param(PROJECT_WITH_TWO_BOARDS_LPPZ,
                                                  id='ProjectWithTwoBoards.lppz')
