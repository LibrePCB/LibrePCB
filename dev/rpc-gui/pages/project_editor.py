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
        self.app.call("ProjectEditor.open", {"path": path})

    def list_clicked(self):
        self.app.call("ProjectEditor.list", {}, self._on_list)

    def get_clicked(self, index):
        self._select(index)
        self.app.call("ProjectEditor.get", {"obj": self._handles[index]},
                      self._on_details)

    def save_clicked(self, index):
        self._select(index)
        self.app.call("ProjectEditor.save", {"obj": self._handles[index]})

    def close_clicked(self, index):
        self._select(index)
        self.app.call("ProjectEditor.close", {"obj": self._handles[index]})

    def _select(self, index):
        # Make the handle of the last used row available to other pages.
        self.app.undo_handle = str(self._handles[index])

    def _on_list(self, error, result):
        if error is not None:
            return
        projects = result.get("projects", [])
        self._handles = [project["obj"] for project in projects]
        self.app.projects = slint.ListModel([
            {
                "name": project["name"],
                "path": project["path"],
                "dirty": project["dirty"],
            }
            for project in projects
        ])

    def _on_details(self, error, result):
        if error is None:
            self.app.project_details = fmt_details(result)
