"""Controls for the ProjectEditor class."""

import slint

from client import fmt_details


class ProjectEditorPage:
    def __init__(self, app):
        self.app = app
        self._handles = []
        self._details_obj = None  # The handle of the last get/set project.
        self._details = None  # The details of the last get/set project.

    # --- Command handlers ----------------------------------------------

    def open_clicked(self):
        path = self.app.project_path.strip()
        if not path:
            return
        self.app.call("ProjectEditor/open", {"path": path},
                      self._on_opened)

    def list_clicked(self):
        self.app.call("ProjectEditor/list", {}, self._on_list)

    def get_clicked(self, index):
        self._select(index)
        self.app.call(
            "ProjectEditor/get", {"obj": self._handles[index]},
            lambda status, payload: self._on_details(
                status, payload, self._handles[index]))

    def save_clicked(self, index):
        self._select(index)
        self.app.call("ProjectEditor.save", {"obj": self._handles[index]})

    def close_clicked(self, index):
        self._select(index)
        self.app.call("ProjectEditor.close", {"obj": self._handles[index]},
                      self._on_closed)

    def apply_clicked(self):
        """Send only the fields which were modified by the user (absent
        arguments are not modified by the server)."""
        if self._details is None:
            return
        params = {}
        for key, prop in [("name", "pe_name"), ("author", "pe_author"),
                          ("version", "pe_version")]:
            value = str(getattr(self.app, prop)).strip()
            if value != self._details.get(key, ""):
                params[key] = value
        for key, prop in [("locale_order", "pe_locales"),
                          ("norm_order", "pe_norms")]:
            value = _split_list(str(getattr(self.app, prop)))
            if value != self._details.get(key, []):
                params[key] = value
        # Use the handle of the project whose details are currently
        # displayed (filled by ProjectEditor/get).
        if params and (self._details_obj is not None):
            self.app.call(
                "ProjectEditor/set", {"obj": self._details_obj, **params},
                lambda status, payload: self._on_details(
                    status, payload, self._details_obj))

    # --- Response handlers ---------------------------------------------

    def _select(self, index):
        # Make the handle of the last used row available to other pages.
        self.app._pages["UndoStack"].select(index)

    def _on_opened(self, status, payload):
        # A single method call per button: open only opens. The user can
        # click List to refresh the project list and Get to fill the
        # properties / DRC entries of a project.
        pass

    def _on_closed(self, status, payload):
        # A single method call per button: close only closes.
        pass

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

    def _on_details(self, status, payload, obj=None):
        if status != 0:
            return
        if obj is not None:
            self._details_obj = obj
        self._details = payload
        self.app.project_details = fmt_details(payload)
        # Pre-fill the set form (also the baseline for the diff).
        self.app.pe_name = payload.get("name", "")
        self.app.pe_author = payload.get("author", "")
        self.app.pe_version = payload.get("version", "")
        self.app.pe_locales = ", ".join(payload.get("locale_order", []))
        self.app.pe_norms = ", ".join(payload.get("norm_order", []))


def _split_list(text):
    return [part.strip() for part in text.split(",") if part.strip()]
