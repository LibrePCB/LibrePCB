"""LibrePCB RPC GUI — a manual testing tool for the RPC server.

The window consists of a sidebar with the connection controls and the
RPC classes (generated list, see api.py), the page of the selected class
and a log. The logic per class lives in pages/*.py, one module per page.
"""

import os
import queue
import time
from datetime import timedelta

import slint

import api
from client import RpcClient, fmt_value, fmt_uuid
from pages.protocol import ProtocolPage
from pages.project_editor import ProjectEditorPage
from pages.rule_check import RuleCheckPage
from pages.undo_stack import UndoStackPage
from pages.remote_library import RemoteLibraryPage
from pages.workspace_db import WorkspaceDbPage

# A project to pre-fill the project path edit with (developer comfort).
DEFAULT_PROJECT_PATH = os.path.normpath(os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..", "..", "tests", "data", "projects", "Gerber Test",
    "project.lpp",
))

# The page index for each class of api.CLASSES (unknown classes get the
# "no page yet" placeholder).
# The page index of each class of api.CLASSES (classes without a page
# show the "no page yet" placeholder). Note: The slint page area refers
# to these fixed indices, so they must not depend on the order of
# api.CLASSES.
PAGES = {
    "Protocol": 0,
    "ProjectEditor": 1,
    "RuleCheck": 2,
    "UndoStack": 3,
    "RemoteLibrary": 4,
    "WorkspaceDb": 5,
}

ns = slint.load_file("app.slint")


class Gui(ns.AppWindow):
    def __init__(self):
        super().__init__()
        self._auto_connect = True
        self.classes = slint.ListModel(api.CLASSES)
        self.current_page = PAGES["ProjectEditor"]
        if os.path.isfile(DEFAULT_PROJECT_PATH):
            self.project_path = DEFAULT_PROJECT_PATH
        self.client = None
        self._log_lines = []
        self.log = ""
        self.projects = slint.ListModel([])
        self.libraries = slint.ListModel([])
        self.project_entries = slint.ListModel([])
        self.undo_handle_valid = False
        self.check_entries = slint.ListModel([])
        self.check_messages = slint.ListModel([])
        self.quick = False
        self._pages = {
            "Protocol": ProtocolPage(self),
            "ProjectEditor": ProjectEditorPage(self),
            "RuleCheck": RuleCheckPage(self),
            "UndoStack": UndoStackPage(self),
            "RemoteLibrary": RemoteLibraryPage(self),
            "WorkspaceDb": WorkspaceDbPage(self),
        }
        self.timer = slint.Timer()
        self.timer.start(
            slint.TimerMode.Repeated, timedelta(milliseconds=50), self.poll
        )
        # Try to connect automatically every second (until the user clicks
        # the connect button manually, which switches to manual mode).
        self.connect_timer = slint.Timer()
        self.connect_timer.start(
            slint.TimerMode.Repeated, timedelta(seconds=1), self.auto_connect
        )

    # --- Toolbar -----------------------------------------------------------

    def auto_connect(self):
        """Try to connect (silently) while the application is not
        running. Skipped once the user clicked the connect button
        manually, which switches to manual mode."""
        if not self._auto_connect or self.client:
            return
        try:
            self.client = RpcClient()
        except OSError:
            return  # The application is not running (yet).
        self.connected = True
        self._log("  Connected. Now initialize to authenticate.")

    @slint.callback
    def connect_clicked(self):
        # Manual mode: no more automatic connection attempts.
        self._auto_connect = False
        self.connect_timer.stop()
        if self.client:
            self.client.close()
            self.client = None
            self.connected = False
            self.authenticated = False
            self.pending = False
            self._log("  Disconnected.")
            return
        try:
            self.client = RpcClient()
            self.connected = True
            self._log("  Connected. Now initialize to authenticate.")
        except OSError as e:
            self._log(f"  Connection failed: {e}")

    @slint.callback
    def init_clicked(self):
        if self.client.pending_msgid is not None:
            try:
                self.client.cancel(self.client.pending_msgid)
            except OSError:
                pass
            self._log(f"⊗ cancel: {self.client.pending_msgid}")
            return
        params = {"name": "librepcb-rpc-gui"}
        token = self.token.strip()
        if token:
            params["token"] = token
        try:
            self.client.pending_msgid = self.call("initialize", params)
            self.pending = True
        except OSError as e:
            self._log(f"  Send failed: {e}")

    @slint.callback
    def select_class(self, index):
        self.selected_class = index
        self.current_page = PAGES.get(api.CLASSES[index], -1)

    # --- Protocol page ----------------------------------------------------

    @slint.callback
    def ping_clicked(self):
        self._pages["Protocol"].ping_clicked()

    # --- ProjectEditor page -----------------------------------------------

    @slint.callback
    def pe_open_clicked(self):
        self._pages["ProjectEditor"].open_clicked()

    @slint.callback
    def pe_list_clicked(self):
        self._pages["ProjectEditor"].list_clicked()

    @slint.callback
    def pe_get_clicked(self, index):
        self._pages["ProjectEditor"].get_clicked(index)

    @slint.callback
    def pe_save_clicked(self, index):
        self._pages["ProjectEditor"].save_clicked(index)

    @slint.callback
    def pe_close_clicked(self, index):
        self._pages["ProjectEditor"].close_clicked(index)

    @slint.callback
    def pe_apply_clicked(self):
        self._pages["ProjectEditor"].apply_clicked()

    # --- RuleCheck page ------------------------------------------------------

    @slint.callback
    def rc_select_check(self, value):
        self._pages["RuleCheck"].select_check(value)

    @slint.callback
    def rc_run_clicked(self):
        self._pages["RuleCheck"].run_clicked()

    @slint.callback
    def rc_cancel_clicked(self):
        self._pages["RuleCheck"].cancel_clicked()

    @slint.callback
    def rc_check_toggled(self, index, checked):
        self._pages["RuleCheck"].check_toggled(index, checked)

    @slint.callback
    def rc_approve_clicked(self):
        self._pages["RuleCheck"].approve_clicked()

    @slint.callback
    def rc_unapprove_clicked(self):
        self._pages["RuleCheck"].unapprove_clicked()

    # --- UndoStack page ----------------------------------------------------

    @slint.callback
    def undo_select_project(self, value):
        self._pages["UndoStack"].select_project(value)

    @slint.callback
    def undo_get_clicked(self):
        self._pages["UndoStack"].get_clicked()

    @slint.callback
    def undo_undo_clicked(self):
        self._pages["UndoStack"].undo_clicked()

    @slint.callback
    def undo_redo_clicked(self):
        self._pages["UndoStack"].redo_clicked()

    # --- RemoteLibrary page ------------------------------------------------

    @slint.callback
    def rl_list_clicked(self):
        self._pages["RemoteLibrary"].list_clicked()

    @slint.callback
    def rl_check_toggled(self, index, checked):
        self._pages["RemoteLibrary"].check_toggled(index, checked)

    @slint.callback
    def rl_apply_clicked(self):
        self._pages["RemoteLibrary"].apply_clicked()

    @slint.callback
    def rl_get_clicked(self, index):
        self._pages["RemoteLibrary"].get_clicked(index)

    # --- WorkspaceDb page --------------------------------------------------

    @slint.callback
    def db_wait_clicked(self):
        self._pages["WorkspaceDb"].wait_clicked()

    # --- RPC plumbing --------------------------------------------------------

    def call(self, method, params, handler=None):
        """Send a request (only if connected). The response is routed to
        the handler and logged either way. Returns the msgid."""
        if not self.client:
            return None
        try:
            msgid = self.client.call(method, params, handler)
        except OSError as e:
            self._log(f"  Send failed: {e}")
            return None
        self._log(f"▶ {method}: {fmt_value(params)}")
        return msgid

    def poll(self):
        if not self.client:
            return
        while True:
            try:
                received_at, msg = self.client.received.get_nowait()
            except queue.Empty:
                return
            if msg is None:
                self.client = None
                self.connected = False
                self.authenticated = False
                self.pending = False
                self._log("  Connection closed.")
                return
            if isinstance(msg, list) and len(msg) == 3 and msg[0] == 2:
                # A notification for a pending request (e.g. progress).
                method = self.client.handlers.get(msg[1], (None,))[0]
                if method:
                    self._log(f"☼ {method}: {fmt_value(msg[2])}")
                else:
                    self._log(f"☼ {fmt_value(msg[2])}")
                self.progress = msg[2].get("progress", 0)
                continue
            if isinstance(msg, list) and len(msg) == 4 and msg[0] == 1:
                self._handle_response(received_at, msg[1], msg[2], msg[3])
            elif isinstance(msg, list) and len(msg) == 2 and msg[0] == 3:
                # A cancel for one of our requests (sent by the callee).
                self._log(f"⊗ cancel: {msg[1]}")
            else:
                # Any other message (e.g. a method call initiated by the
                # application, which the GUI does not support yet).
                self._log(f"? unhandled: {fmt_value(msg)}")

    def _handle_response(self, received_at, msgid, status, payload):
        method, handler, sent_at = self.client.handlers.pop(
            msgid, (None, None, 0.0)
        )
        if msgid == self.client.pending_msgid:
            # The initialize reply (status 0 or error code) (un-)authenticates.
            self.client.pending_msgid = None
            self.pending = False
            self.authenticated = status == 0
            if status == 0 and "token" in payload:
                self.token = str(payload["token"])
        duration = f" ({int((received_at - sent_at) * 1000)} ms)"
        if status == 0:
            self._log(f"✔ {method}: {fmt_value(payload)}{duration}")
        else:
            message = payload.get("message", "") if isinstance(payload, dict) else ""
            self._log(f"✘ {method}: {status} {message}{duration}")
        if handler:
            handler(status, payload)

    def _log(self, text):
        line = f"[{time.strftime('%H:%M:%S')}] {text}"
        self._log_lines.append(line)
        self._log_lines = self._log_lines[-200:]
        self.log = "\n".join(self._log_lines)


if __name__ == "__main__":
    Gui().run()
