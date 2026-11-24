"""Controls for the WorkspaceDb class."""


class WorkspaceDbPage:
    def __init__(self, app):
        self.app = app

    def wait_clicked(self):
        self.app.call("WorkspaceDb.wait", {}, self._on_done)

    def _on_done(self, error, result):
        # Hide the progress bar again (it showed the scan progress).
        self.app.progress = 0
