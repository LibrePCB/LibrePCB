"""Controls for the RuleCheck class (ERC on projects, DRC on boards)."""

import slint


class RuleCheckPage:
    def __init__(self, app):
        self.app = app
        self._objects = []  # [{"name", "obj", "board"}] checkable objects
        self._messages = []  # The raw result, for row actions.
        self._checked = set()  # Handles of the checked messages.
        self._selected = None  # The handle of the selected object.
        self._selected_project = None  # Its project handle (for set).
        self._running_msgid = None  # The pending RuleCheck/run request.

    # --- Object management (called by the ProjectEditor page) ----------

    def set_projects(self, projects):
        """Update the entries when the project list was fetched: An ERC
        entry per project, keeping the board (DRC) entries of still-open
        projects (their boards are refreshed by the per-project gets)."""
        project_handles = {p["obj"] for p in projects}
        self._objects = [
            entry
            for entry in self._objects
            if entry["project"] in project_handles
        ]
        known = {entry["obj"] for entry in self._objects}
        for p in projects:
            if p["obj"] not in known:
                self._objects.append(
                    {"name": p["name"], "obj": p["obj"],
                     "project": p["obj"], "project_name": p["name"],
                     "board": False})
        self._update_model()

    def set_boards(self, project, boards):
        """Update the board (DRC) entries of the given project, keeping
        the entries of all other projects."""
        project_name = next(
            (entry["name"] for entry in self._objects
             if (entry["obj"] == project) and (not entry["board"])), "")
        self._objects = [
            entry
            for entry in self._objects
            if not (entry["board"] and (entry["project"] == project))
        ]
        # Insert the boards right after the project's ERC entry.
        index = next(
            (i for i, entry in enumerate(self._objects)
             if (entry["obj"] == project) and (not entry["board"])),
            len(self._objects))
        for i, b in enumerate(boards):
            self._objects.insert(index + 1 + i, {
                "name": b["name"], "obj": b["obj"],
                "project": project, "project_name": project_name,
                "board": True})
        self._update_model()

    def select_check(self, value):
        """The dropdown was used to select an entry (by display string)."""
        for entry in self._objects:
            if _label(entry) == value:
                self._selected = entry["obj"]
                self._selected_project = entry.get(
                    "project", entry["obj"])
                return

    # --- Command handlers ----------------------------------------------

    def refresh_clicked(self):
        """Manually rebuild the dropdown from ProjectEditor/list and one
        ProjectEditor/get per project."""
        self.app.call("ProjectEditor/list", {}, self._on_refresh_list)

    def run_clicked(self):
        if (self._selected is None) or self._running_msgid:
            return
        params = {"obj": self._selected}
        if self.app.quick:
            params["quick"] = True
        self.app.rulecheck_running = True
        self.app.progress = 0
        self._running_msgid = self.app.call(
            "RuleCheck/run", params, self._on_run)

    def cancel_clicked(self):
        if self._running_msgid is None:
            return
        try:
            self.app.client.cancel(self._running_msgid)
        except OSError:
            pass

    def check_toggled(self, index, checked):
        handle = self._messages[index]["obj"]
        if checked:
            self._checked.add(handle)
        else:
            self._checked.discard(handle)
        self._update_model()

    def approve_clicked(self):
        self._call_set(True)

    def unapprove_clicked(self):
        self._call_set(False)

    # --- Helpers ---------------------------------------------------------

    def _call_set(self, approved):
        if not self._checked:
            return
        # The approved state lives on the project, so the project handle
        # is the object of this request (even for DRC messages).
        obj = self._selected_project or self._selected
        self.app.call("RuleCheck/set",
                      {"obj": obj,
                       "messages": sorted(self._checked),
                       "approved": approved})

    def _on_refresh_list(self, status, payload):
        if status != 0:
            return
        projects = payload.get("projects", [])
        self.set_projects(projects)
        for project in projects:
            self.app.call(
                "ProjectEditor/get", {"obj": project["obj"]},
                lambda status, payload, project=project:
                self._on_refresh_details(status, payload, project))

    def _on_refresh_details(self, status, payload, project):
        if status != 0:
            return
        boards = [
            {"name": board["name"], "obj": board["obj"]}
            for board in payload.get("boards", [])
        ]
        self.set_boards(project["obj"], boards)

    def _on_run(self, status, payload):
        self._running_msgid = None
        self.app.rulecheck_running = False
        self.app.progress = 0
        if status != 0:
            return
        self._messages = payload.get("messages", [])
        self._checked = set()
        self._update_model()

    def _update_model(self):
        self.app.check_entries = slint.ListModel(
            [_label(entry) for entry in self._objects])
        # Keep the selection if it still exists, otherwise select the
        # first entry (the Run button needs a selection).
        selected = next(
            (entry for entry in self._objects
             if entry["obj"] == self._selected), None)
        if selected is None and self._objects:
            selected = self._objects[0]
        if selected is not None:
            self._selected = selected["obj"]
            self._selected_project = selected.get(
                "project", self._selected)
            self.app.check_index = self._objects.index(selected)
        else:
            self._selected = None
            self._selected_project = None
            self.app.check_index = -1
        self.app.check_messages = slint.ListModel([
            {
                "checked": message["obj"] in self._checked,
                "severity": message.get("severity", ""),
                "message": message.get("message", ""),
                "description": message.get("description") or "",
                "approved": message.get("approved", False),
            }
            for message in self._messages
        ])


def _label(entry):
    if entry["board"]:
        return f"{entry['project_name']}: {entry['name']} (DRC)"
    return f"{entry['name']} (ERC)"
