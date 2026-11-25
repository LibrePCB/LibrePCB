"""Controls for the RuleCheck class (ERC on projects, DRC on boards)."""

import slint


class RuleCheckPage:
    def __init__(self, app):
        self.app = app
        self._objects = []  # [{"name", "obj", "board"}] checkable objects
        self._messages = []  # The raw result, for row actions.
        self._checked = set()  # Handles of the checked messages.
        self._selected = None  # The handle of the selected object.
        self._running_msgid = None  # The pending RuleCheck/run request.

    # --- Object management (called by the ProjectEditor page) ----------

    def set_projects(self, projects):
        """Update the project entries (ERC), keeping boards (DRC) of
        still-open projects."""
        project_handles = {p["obj"] for p in projects}
        self._objects = [
            entry
            for entry in self._objects
            if entry["board"] or entry["obj"] in project_handles
        ]
        known = {entry["obj"] for entry in self._objects}
        for p in projects:
            if p["obj"] not in known:
                self._objects.append(
                    {"name": p["name"], "obj": p["obj"], "board": False})
        self._update_model()

    def set_boards(self, project, boards):
        """Update the board entries (DRC) of the given project."""
        self._objects = [
            entry
            for entry in self._objects
            if (not entry["board"]) or (entry["obj"] in {
                b["obj"] for b in boards})
        ]
        known = {entry["obj"] for entry in self._objects}
        for b in boards:
            if b["obj"] not in known:
                self._objects.append(
                    {"name": b["name"], "obj": b["obj"], "board": True})
        self._update_model()

    def select_check(self, value):
        """The dropdown was used to select an entry (by display string)."""
        for entry in self._objects:
            if _label(entry) == value:
                self._selected = entry["obj"]
                return

    # --- Command handlers ----------------------------------------------

    def run_clicked(self):
        if (self._selected is None) or self._running_msgid:
            return
        params = {"obj": self._selected}
        if self.app.quick:
            params["quick"] = True
        self.app.running = True
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
        self.app.call("RuleCheck/set",
                      {"messages": sorted(self._checked),
                       "approved": approved})

    def _on_run(self, status, payload):
        self._running_msgid = None
        self.app.running = False
        self.app.progress = 0
        if status != 0:
            return
        self._messages = payload.get("messages", [])
        self._checked = set()
        self._update_model()

    def _update_model(self):
        self.app.check_entries = slint.ListModel(
            [_label(entry) for entry in self._objects])
        self.app.messages = slint.ListModel([
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
    return ("DRC: " if entry["board"] else "ERC: ") + entry["name"]
