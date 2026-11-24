"""Controls for the RemoteLibrary class."""

import slint

from client import fmt_details, fmt_uuid


class RemoteLibraryPage:
    def __init__(self, app):
        self.app = app
        self._libraries = []  # The raw result, for row actions.
        self._checked = set()  # UUIDs of the checked libraries.

    def list_clicked(self):
        self.app.call("RemoteLibrary/list", {}, self._on_list)

    def check_toggled(self, index, checked):
        uuid = self._libraries[index]["uuid"]
        if checked:
            self._checked.add(uuid)
        else:
            self._checked.discard(uuid)
        self._update_model()

    def apply_clicked(self):
        """Apply the desired installation state of the checked libraries:
        Checked libraries are installed, installed libraries which are
        unchecked are uninstalled, everything else is left untouched."""
        install = []
        uninstall = []
        for library in self._libraries:
            uuid = library["uuid"]
            if uuid in self._checked:
                install.append(uuid)
            elif library.get("installed_version"):
                uninstall.append(uuid)
        self.app.call("RemoteLibrary/apply",
                      {"install": install, "uninstall": uninstall})

    def get_clicked(self, index):
        # Remote libraries are not registered objects, so there is no
        # handle (yet) — pass the UUID to at least test the error response.
        uuid = self._libraries[index]["uuid"]
        self.app.call("RemoteLibrary/get", {"obj": uuid}, self._on_details)

    def _on_list(self, status, payload):
        if status != 0:
            return
        self._libraries = payload.get("libraries", [])
        # The checkboxes show the desired installation state, initialized
        # with the current state (installed libraries are checked).
        self._checked = {
            library["uuid"]
            for library in self._libraries
            if library.get("installed_version")
        }
        self._update_model()

    def _on_details(self, status, payload):
        if status == 0:
            self.app.library_details = fmt_details(payload)

    def _update_model(self):
        self.app.libraries = slint.ListModel([
            {
                "checked": library["uuid"] in self._checked,
                "id": self._display_id(library),
                "name": library.get("name", ""),
                "installed": library.get("installed_version") or "",
                "online": library.get("online_version") or "",
            }
            for library in self._libraries
        ])

    def _display_id(self, library):
        """The identifier shown in the list: The path if the library has
        one, otherwise its UUID."""
        path = library.get("path")
        return path if path else fmt_uuid(library["uuid"])
