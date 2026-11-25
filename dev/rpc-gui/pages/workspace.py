"""Controls for the Workspace class."""

from client import fmt_details


class WorkspacePage:
    def __init__(self, app):
        self.app = app

    def get_clicked(self):
        self.app.call("Workspace/get", {}, self._on_get)

    def _on_get(self, status, payload):
        if status == 0:
            self.app.workspace_details = fmt_details(payload)
