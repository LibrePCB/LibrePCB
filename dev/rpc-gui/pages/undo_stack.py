"""Controls for the UndoStack class."""

import slint

from client import fmt_details


class UndoStackPage:
    def __init__(self, app):
        self.app = app
        self._projects = []  # [{"name": …, "obj": …}] from ProjectEditor.list
        self._handle = None  # The handle of the selected project.

    def set_projects(self, projects):
        """Update the project dropdown (called by the ProjectEditor page)."""
        self._projects = projects
        self.app.project_entries = slint.ListModel([
            f"{project['name']} ({project['obj']})"
            for project in projects
        ])
        handles = [project["obj"] for project in projects]
        if self._handle not in handles:
            self.select(0 if handles else -1)
        self._update_state()

    def select(self, index):
        """Select the project with the given index (row actions do this)."""
        if 0 <= index < len(self._projects):
            self._handle = self._projects[index]["obj"]
        else:
            self._handle = None
        self._update_state()

    def select_project(self, value):
        """The dropdown was used to select an entry (by display string)."""
        for index, project in enumerate(self._projects):
            if f"{project['name']} ({project['obj']})" == value:
                self.select(index)
                return

    def get_clicked(self):
        self._call("UndoStack.get")

    def undo_clicked(self):
        self._call("UndoStack.undo")

    def redo_clicked(self):
        self._call("UndoStack.redo")

    def _call(self, method):
        if self._handle is None:
            return
        self.app.call(method, {"obj": self._handle}, self._on_details)

    def _on_details(self, status, payload):
        if status == 0:
            self.app.undo_details = fmt_details(payload)

    def _update_state(self):
        index = -1
        for i, project in enumerate(self._projects):
            if project["obj"] == self._handle:
                index = i
                break
        self.app.undo_project_index = index
        self.app.undo_handle_valid = self._handle is not None
