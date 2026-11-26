"""Controls for the WorkspaceDb class."""

import uuid as uuidlib

from client import fmt_details


class WorkspaceDbPage:
    def __init__(self, app):
        self.app = app

    def wait_clicked(self):
        self.app.call("WorkspaceDb/wait", {}, self._on_done)

    def query_clicked(self):
        query = self.app.db_query.strip()
        if not query:
            return
        self.app.call("WorkspaceDb/query_sql", {"query": query},
                      self._on_result)

    def get_clicked(self):
        value = self.app.db_get_input.strip()
        if not value:
            return
        params = {"type": self.app.db_get_type}
        try:
            # UUIDs are plain strings now: If the input is a valid UUID,
            # it is sent in the 'uuid' argument instead of 'path'.
            uuidlib.UUID(value)
            params["uuid"] = value
        except ValueError:
            params["path"] = value
        self.app.call("WorkspaceDb/get", params, self._on_result)

    def _on_done(self, status, payload):
        # Hide the progress bar again (it showed the scan progress).
        self.app.progress = 0

    def _on_result(self, status, payload):
        # Print the formatted response data 1:1 (like ProjectEditor/get).
        self.app.db_details = fmt_details(payload)
