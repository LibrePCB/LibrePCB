"""Controls for the WorkspaceDb class."""

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
                      self._on_query)

    def _on_done(self, status, payload):
        # Hide the progress bar again (it showed the scan progress).
        self.app.progress = 0

    def _on_query(self, status, payload):
        # Print the formatted response data 1:1 (like ProjectEditor/get).
        self.app.db_details = fmt_details(payload)
