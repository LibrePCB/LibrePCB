"""LibrePCB RPC GUI — a manual testing tool for the RPC server.

The window consists of a connection toolbar, a sidebar with the RPC
classes (generated list, see api.py), the page of the selected class and
a log. The logic per class lives in pages/*.py, one module per page.
"""

import queue
import time
from datetime import timedelta

import slint

import api
from client import RpcClient, fmt_value
from pages.protocol import ProtocolPage
from pages.project_editor import ProjectEditorPage
from pages.undo_stack import UndoStackPage
from pages.remote_library import RemoteLibraryPage
from pages.workspace_db import WorkspaceDbPage

# The page index for each class of api.CLASSES (unknown classes get the
# "no page yet" placeholder).
PAGES = {
    "Protocol": 0,
    "ProjectEditor": 1,
    "UndoStack": 2,
    "RemoteLibrary": 3,
    "WorkspaceDb": 4,
}

ns = slint.load_file("app.slint")


class Gui(ns.AppWindow):
    def __init__(self):
        super().__init__()
        self.classes = slint.ListModel(api.CLASSES)
        self.current_page = PAGES["ProjectEditor"]
        self.client = None
        self._log_lines = []
        self.log = ""
        self.projects = slint.ListModel([])
        self.libraries = slint.ListModel([])
        self._pages = {
            "Protocol": ProtocolPage(self),
            "ProjectEditor": ProjectEditorPage(self),
            "UndoStack": UndoStackPage(self),
            "RemoteLibrary": RemoteLibraryPage(self),
            "WorkspaceDb": WorkspaceDbPage(self),
        }
        self.timer = slint.Timer()
        self.timer.start(
            slint.TimerMode.Repeated, timedelta(milliseconds=50), self.poll
        )

    # --- Toolbar -----------------------------------------------------------

    @slint.callback
    def connect_clicked(self):
        if self.client:
            self.client.close()
            self.client = None
            self.connected = False
            self.authenticated = False
            self.pending = False
            self._log("Disconnected.")
            return
        try:
            self.client = RpcClient()
            self.connected = True
            self._log("Connected. Now initialize to authenticate.")
        except OSError as e:
            self._log(f"Connection failed: {e}")

    @slint.callback
    def init_clicked(self):
        if self.client.pending_msgid is not None:
            try:
                self.client.cancel(self.client.pending_msgid)
            except OSError:
                pass
            self.client.pending_msgid = None
            self.pending = False
            self._log("Cancelled.")
            return
        params = {"name": "librepcb-rpc-gui"}
        token = self.token.strip()
        if token:
            params["token"] = token
        try:
            self.client.pending_msgid = self.client.call(
                "initialize", params
            )
            self.pending = True
        except OSError as e:
            self._log(f"Send failed: {e}")

    @slint.callback
    def select_class(self, index):
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

    # --- UndoStack page ----------------------------------------------------

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
    def rl_install_clicked(self):
        self._pages["RemoteLibrary"].install_clicked()

    @slint.callback
    def rl_uninstall_clicked(self):
        self._pages["RemoteLibrary"].uninstall_clicked()

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
        the handler and logged either way."""
        if not self.client:
            return
        try:
            self.client.call(method, params, handler)
        except OSError as e:
            self._log(f"Send failed: {e}")
        else:
            self._log(f"→ {method}")

    def poll(self):
        if not self.client:
            return
        while True:
            try:
                msg = self.client.received.get_nowait()
            except queue.Empty:
                return
            if msg is None:
                self.client = None
                self.connected = False
                self.authenticated = False
                self.pending = False
                self._log("Connection closed.")
                return
            if isinstance(msg, list) and len(msg) == 3 and msg[0] == 3:
                self.progress = msg[2].get("progress", 0) * 100
                continue
            if isinstance(msg, list) and len(msg) == 4 and msg[0] == 1:
                self._handle_response(msg[1], msg[2], msg[3])
            elif isinstance(msg, list) and len(msg) == 3 and msg[0] == 2:
                self._log(f"⇐ notification: {fmt_value(msg[2])}")

    def _handle_response(self, msgid, error, result):
        method, handler = self.client.handlers.pop(msgid, (None, None))
        if msgid == self.client.pending_msgid:
            # The initialize reply (success/error) (un-)authenticates.
            self.client.pending_msgid = None
            self.pending = False
            self.authenticated = error is None
            if error is None and "token" in result:
                self.token = str(result["token"])
        if error is None:
            self._log(f"← {method}: {fmt_value(result)}")
        else:
            self._log(f"← ⚠ {method}: {error.get('code')} {error.get('message')}")
        if handler:
            handler(error, result)

    def _log(self, text):
        line = f"{time.strftime('%H:%M:%S')}  {text}"
        self._log_lines.append(line)
        self._log_lines = self._log_lines[-200:]
        self.log = "\n".join(self._log_lines)


if __name__ == "__main__":
    Gui().run()
