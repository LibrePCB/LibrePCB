"""Throwaway verification of the obj-handle fixes (deleted after
running)."""

import os
import sys

os.chdir(os.path.join(os.path.dirname(__file__), "..", "..", "..",
                       "dev", "rpc-gui"))

sys.path.insert(0, os.getcwd())

import umsgpack  # noqa: E402
from main import Gui  # noqa: E402


def ext_uuid(hexstr):
    return umsgpack.Ext(2, bytes.fromhex(hexstr.replace("-", "")))


def test_obj_handles():
    gui = Gui()
    rc = gui._pages["RuleCheck"]
    calls = []
    gui.call = lambda m, p, h=None: calls.append((m, p))

    # RuleCheck/set must include the (project) obj handle.
    rc.set_projects([{"name": "Proj", "obj": 7}])
    assert rc._selected_project == 7
    rc._on_run(0, {"messages": [
        {"obj": "h1", "severity": "error", "message": "Boom",
         "description": None, "approved": False}]})
    rc.check_toggled(0, True)
    rc.approve_clicked()
    assert calls == [("RuleCheck/set",
                      {"obj": 7, "messages": ['h1'], 'approved': True})], calls

    # DRC entry selected: set still uses the project handle.
    rc.set_boards(7, [{"name": "Board", "obj": 9}])
    rc.select_check("DRC: Board")
    assert rc._selected == 9 and rc._selected_project == 7
    rc.unapprove_clicked()
    assert calls[-1] == ("RuleCheck/set",
                         {"obj": 7, "messages": ['h1'],
                          'approved': False}), calls[-1]

    # ProjectEditor/set: unknown project handle -> no send (yet).
    pe = gui._pages["ProjectEditor"]
    pe._on_details(0, {"name": "P", "author": "A", "version": "v1",
                       "locale_order": [], "norm_order": []})
    gui.pe_author = "A2"
    pe.apply_clicked()
    assert calls[-1] == ("RuleCheck/set", calls[-1][1]), "no send expected"

    # ...and with the list known (single project): obj auto-associated.
    pe._on_list(0, {"projects": [{"name": "P", "obj": 7, "path": "",
                                  "dirty": False}]})
    gui.pe_author = "A"  # Revert the earlier edit.
    gui.pe_version = "v2"
    pe.apply_clicked()
    assert calls[-1] == ("ProjectEditor/set",
                         {"obj": 7, "version": "v2"}), calls[-1]
    print("OBJ HANDLES OK")
