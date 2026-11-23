import queue
import socket
import threading
from datetime import timedelta

import slint
import umsgpack

PORT = 19536


class RpcClient:
    def __init__(self):
        self.sock = socket.create_connection(("127.0.0.1", PORT))
        self.file = self.sock.makefile("rb")
        self.msgid = 0
        self.pending_msgid = None
        self.responses = queue.Queue()
        threading.Thread(target=self.read_loop, daemon=True).start()

    def read_loop(self):
        while True:
            try:
                msg = umsgpack.unpack(self.file)
            except Exception:
                msg = None
            self.responses.put(msg)
            if msg is None:
                break

    def call(self, method, params):
        self.msgid += 1
        msgid = self.msgid
        self.sock.sendall(umsgpack.packb([0, msgid, method, params]))
        return msgid

    def cancel(self, msgid):
        self.sock.sendall(umsgpack.packb([4, msgid]))


ns = slint.load_file("app.slint")


class Gui(ns.AppWindow):
    def __init__(self):
        super().__init__()
        self._log_lines = []
        self.log = ""
        self._project_handles = []
        self.client = None
        self.timer = slint.Timer()
        self.timer.start(
            slint.TimerMode.Repeated, timedelta(milliseconds=50), self.poll
        )

    @slint.callback
    def connect_clicked(self):
        if self.client:
            self.client.sock.close()
            self.client = None
            self.connected = False
            self.pending = False
            self._log("Disconnected.")
            return
        try:
            self.client = RpcClient()
            self.connected = True
            self._log("Connected.")
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
            self.client.pending_msgid = self.client.call("initialize", params)
            self.pending = True
        except OSError as e:
            self._log(f"Connection failed: {e}")

    @slint.callback
    def open_project(self):
        if not self.client:
            return
        path = self.project_path.strip()
        if not path:
            self._log("No project path given.")
            return
        try:
            self.client.call("ProjectEditor.open", {"path": path})
        except OSError as e:
            self._log(f"Connection failed: {e}")

    @slint.callback
    def list_projects(self):
        if not self.client:
            return
        try:
            self.client.call("ProjectEditor.list", {})
        except OSError as e:
            self._log(f"Connection failed: {e}")

    @slint.callback
    def close_project(self, index):
        if not self.client:
            return
        try:
            obj = self._project_handles[index]
        except IndexError:
            return
        try:
            self.client.call("ProjectEditor.close", {"obj": obj})
        except OSError as e:
            self._log(f"Connection failed: {e}")

    @slint.callback
    def watch_clicked(self):
        if not self.client:
            return
        try:
            self.client.call("WorkspaceDb.watch", {})
        except OSError as e:
            self._log(f"Connection failed: {e}")

    def _log(self, msg):
        self._log_lines.append(msg)
        self._log_lines = self._log_lines[-100:]
        self.log = "\n".join(self._log_lines)

    def poll(self):
        if not self.client:
            return
        while True:
            try:
                msg = self.client.responses.get_nowait()
            except queue.Empty:
                return
            if msg is None:
                self.client = None
                self.connected = False
                self.pending = False
                self._log("Connection closed.")
                return
            if isinstance(msg, list) and len(msg) == 3 and msg[0] == 3:
                payload = msg[2]
                self.progress = payload.get("progress", 0) * 100
                self._log(f"Progress: {self.progress}%")
                continue
            if isinstance(msg, list) and len(msg) == 4 and msg[0] == 1:
                msgid, error, result = msg[1], msg[2], msg[3]
                if msgid == self.client.pending_msgid:
                    self.client.pending_msgid = None
                    self.pending = False
                if error is None:
                    self._log(f"Result: {result}")
                    if 'projects' in result:
                        self._project_handles = [
                            project['obj'] for project in result['projects']
                        ]
                        self.projects = slint.ListModel([
                            {
                                'name': project['name'],
                                'path': project['path'],
                                'dirty': project['dirty'],
                            }
                            for project in result['projects']
                        ])
                    if 'token' in result:
                        self.token = str(result['token'])
                else:
                    self._log(
                        f"Error {error.get('code')}: {error.get('message')}"
                    )


if __name__ == "__main__":
    Gui().run()
