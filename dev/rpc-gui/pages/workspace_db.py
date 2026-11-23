"""Controls for the WorkspaceDb class."""


class WorkspaceDbPage:
    def __init__(self, app):
        self.app = app

    def wait_clicked(self):
        self.app.call("WorkspaceDb.wait", {})
