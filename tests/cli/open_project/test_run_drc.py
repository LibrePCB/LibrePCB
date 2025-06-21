#!/usr/bin/env python
# -*- coding: utf-8 -*-

import params
import pytest
from helpers import nofmt

"""
Test command "open-project --drc"
"""


@pytest.mark.parametrize("project", [params.EMPTY_PROJECT_LPP_PARAM])
def test_if_project_without_boards_succeeds(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    # remove all boards
    with open(cli.abspath(project.dir + "/boards/boards.lp"), "w") as f:
        f.write("(librepcb_boards)")
    code, stdout, stderr = cli.run("open-project", "--drc", project.path)
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Run DRC...
SUCCESS
""")
    assert code == 0


@pytest.mark.parametrize("project", [params.PROJECT_WITH_TWO_BOARDS_LPPZ_PARAM])
def test_project_with_two_boards_explicit_one(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run(
        "open-project", "--drc", "--board=copy", project.path
    )
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Run DRC...
  Board 'copy':
    Approved messages: 0
    Non-approved messages: 0
SUCCESS
""")
    assert code == 0


@pytest.mark.parametrize("project", [params.EMPTY_PROJECT_LPP_PARAM])
def test_board_with_approved_message(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    # Add a hole with small diameter and the corresponding approval.
    with open(cli.abspath(project.dir + "/boards/default/board.lp"), "r") as f:
        board_content = f.read()
    board_content = board_content.replace(
        """
)
""",
        """
 (hole 82506db2-3323-4732-8480-f3517f10dd44
  (diameter 0.1) (stop_mask auto) (lock false)
  (vertex (position 50.0 50.0) (angle 0.0))
 )
)
""",
    )
    board_content = board_content.replace(
        """
 (design_rule_check
""",
        """
 (design_rule_check
  (approved minimum_drill_diameter_violation
   (hole 82506db2-3323-4732-8480-f3517f10dd44)
  )
""",
    )
    with open(cli.abspath(project.dir + "/boards/default/board.lp"), "w") as f:
        f.write(board_content)
    code, stdout, stderr = cli.run("open-project", "--drc", project.path)
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Run DRC...
  Board 'default':
    Approved messages: 1
    Non-approved messages: 0
SUCCESS
""")
    assert code == 0


@pytest.mark.parametrize("project", [params.EMPTY_PROJECT_LPP_PARAM])
def test_board_without_messages(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    code, stdout, stderr = cli.run("open-project", "--drc", project.path)
    assert stderr == ""
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Run DRC...
  Board 'default':
    Approved messages: 0
    Non-approved messages: 0
SUCCESS
""")
    assert code == 0


@pytest.mark.parametrize("project", [params.EMPTY_PROJECT_LPP_PARAM])
def test_board_with_nonapproved_message(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    # Add a hole with small diameter.
    with open(cli.abspath(project.dir + "/boards/default/board.lp"), "r") as f:
        board_content = f.read()
    board_content = board_content.replace(
        """
)
""",
        """
 (hole 82506db2-3323-4732-8480-f3517f10dd44
  (diameter 0.1) (stop_mask auto) (lock false)
  (vertex (position 50.0 50.0) (angle 0.0))
 )
)
""",
    )
    with open(cli.abspath(project.dir + "/boards/default/board.lp"), "w") as f:
        f.write(board_content)
    code, stdout, stderr = cli.run("open-project", "--drc", project.path)
    assert stderr == nofmt("""\
      - [WARNING] NPTH drill diameter: 0.1 < 0.25 mm
""")
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Run DRC...
  Board 'default':
    Approved messages: 0
    Non-approved messages: 1
Finished with errors!
""")
    assert code == 1


@pytest.mark.parametrize("project", [params.EMPTY_PROJECT_LPP_PARAM])
def test_with_custom_settings(cli, project):
    cli.add_project(project.dir, as_lppz=project.is_lppz)
    settings = """
      (design_rule_check
       (min_copper_copper_clearance 0.2)
       (min_copper_board_clearance 0.3)
       (min_copper_npth_clearance 0.2)
       (min_drill_drill_clearance 0.35)
       (min_drill_board_clearance 0.5)
       (min_silkscreen_stopmask_clearance 0.127)
       (min_copper_width 0.2)
       (min_annular_ring 0.15)
       (min_npth_drill_diameter 0.1234)
       (min_pth_drill_diameter 0.25)
       (min_npth_slot_width 1.0)
       (min_pth_slot_width 0.7)
       (min_silkscreen_width 0.1)
       (min_silkscreen_text_height 0.8)
       (min_outline_tool_diameter 2.0)
       (blind_vias_allowed true)
       (buried_vias_allowed true)
       (allowed_npth_slots single_segment_straight)
       (allowed_pth_slots single_segment_straight)
       (approvals_version "0.2")
      )
    """
    with open(cli.abspath("settings.lp"), mode="w") as f:
        f.write(settings)
    # Add a hole with small diameter.
    with open(cli.abspath(project.dir + "/boards/default/board.lp"), "r") as f:
        board_content = f.read()
    board_content = board_content.replace(
        """
)
""",
        """
 (hole 82506db2-3323-4732-8480-f3517f10dd44
  (diameter 0.1) (stop_mask auto) (lock false)
  (vertex (position 50.0 50.0) (angle 0.0))
 )
)
""",
    )
    with open(cli.abspath(project.dir + "/boards/default/board.lp"), "w") as f:
        f.write(board_content)
    code, stdout, stderr = cli.run(
        "open-project", "--drc", "--drc-settings", "settings.lp", project.path
    )
    assert stderr == nofmt("""\
      - [WARNING] NPTH drill diameter: 0.1 < 0.1234 mm
""")
    assert stdout == nofmt(f"""\
Open project '{project.path}'...
Run DRC...
  Board 'default':
    Approved messages: 0
    Non-approved messages: 1
Finished with errors!
""")
    assert code == 1
