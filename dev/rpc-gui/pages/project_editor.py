"""Controls for the ProjectEditor class."""

import slint

from client import fmt_details


class ProjectEditorPage:
    def __init__(self, app):
        self.app = app
        self._handles = []

    def open_clicked(self):
        path = self.app.project_path.strip()
        if not path:
            return
        self.app.call("ProjectEditor/open", {"path": path}, self._on_opened)

    def _on_opened(self, status, payload):
        # Opening changes the project list, so refresh it automatically.
        if status == 0:
            self.list_clicked()

    def list_clicked(self):
        self.app.call("ProjectEditor/list", {}, self._on_list)

    def get_clicked(self, index):
        self._select(index)
        self.app.call("ProjectEditor/get", {"obj": self._handles[index]},
                      self._on_details)

    def save_clicked(self, index):
        self._select(index)
        self.app.call("ProjectEditor/save", {"obj": self._handles[index]})

    def close_clicked(self, index):
        self._select(index)
        self.app.call("ProjectEditor/close", {"obj": self._handles[index]},
                      self._on_closed)

    def _on_closed(self, status, payload):
        # Closing changes the project list, so refresh it automatically.
        if status == 0:
            self.list_clicked()

    def _select(self, index):
        # Make the handle of the last used row available to other pages.
        self.app._pages["UndoStack"].select(index)

    def _on_list(self, status, payload):
        if status != 0:
            return
        projects = payload.get("projects", [])
        self._handles = [project["obj"] for project in projects]
        self.app.projects = slint.ListModel([
            {
                "name": project["name"],
                "path": project["path"],
                "dirty": project["dirty"],
            }
            for project in projects
        ])
        self.app._pages["UndoStack"].set_projects(projects)

    def _on_details(self, status, payload):
        if status == 0:
            self.app.project_details = fmt_details(payload)
