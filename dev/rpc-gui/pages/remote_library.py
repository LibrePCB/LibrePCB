"""Controls for the RemoteLibrary class."""

import slint

from client import fmt_details


class RemoteLibraryPage:
    def __init__(self, app):
        self.app = app
        self._libraries = []  # The raw result, for row actions.
        self._checked = set()  # UUIDs of the checked libraries.

    def list_clicked(self):
        self.app.call("RemoteLibrary.list", {}, self._on_list)

    def check_toggled(self, index, checked):
        uuid = self._libraries[index]["uuid"]
        if checked:
            self._checked.add(uuid)
        else:
            self._checked.discard(uuid)
        self._update_model()

    def install_clicked(self):
        self._call_batch("RemoteLibrary.install")

    def uninstall_clicked(self):
        self._call_batch("RemoteLibrary.uninstall")

    def get_clicked(self, index):
        # Remote libraries are not registered objects, so there is no
        # handle (yet) — pass the UUID to at least test the error response.
        uuid = self._libraries[index]["uuid"]
        self.app.call("RemoteLibrary.get", {"obj": uuid}, self._on_details)

    def _call_batch(self, method):
        if not self._checked:
            return
        self.app.call(method, {"libraries": sorted(self._checked)})

    def _on_list(self, status, payload):
        if status != 0:
            return
        self._libraries = payload.get("libraries", [])
        self._update_model()

    def _on_details(self, status, payload):
        if status == 0:
            self.app.library_details = fmt_details(payload)

    def _update_model(self):
        self.app.libraries = slint.ListModel([
            {
                "checked": library["uuid"] in self._checked,
                "uuid": library.get("uuid", ""),
                "name": library.get("name", ""),
                "installed": library.get("installed_version", ""),
                "online": library.get("online_version", ""),
                "outdated": library.get("outdated", False),
            }
            for library in self._libraries
        ])
