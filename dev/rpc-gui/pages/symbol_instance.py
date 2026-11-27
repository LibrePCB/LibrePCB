"""Controls for the SymbolInstance class."""

import slint

from client import fmt_details


class SymbolInstancePage:
    def __init__(self, app):
        self.app = app
        self._schematics = {}  # Schematic display name → obj handle.

    # --- Object management (schematic handles, like RuleCheck) ----------

    def refresh_clicked(self):
        """Rebuild the schematic dropdown with one ProjectEditor/list and
        one ProjectEditor/get per project."""
        self.app.call("ProjectEditor/list", {}, self._on_refresh_list)

    def _on_refresh_list(self, status, payload):
        if status != 0:
            return
        self._schematics = {}
        self._update_model()
        for project in payload.get("projects", []):
            self.app.call(
                "ProjectEditor/get", {"obj": project["obj"]},
                lambda status, payload, project=project:
                self._on_refresh_details(status, payload, project))

    def _on_refresh_details(self, status, payload, project):
        if status != 0:
            return
        for sheet in payload.get("sheets", []):
            self._schematics[f"{project['name']}: {sheet['name']}"] = \
                sheet["obj"]
        self._update_model()

    def _update_model(self):
        self.app.si_entries = slint.ListModel(self._schematics.keys())
        if self._schematics and (self.app.si_index < 0):
            self.app.si_index = 0

    def select_schematic(self, value):
        # No action needed: The obj handle is looked up by display name.
        pass

    # --- Command handlers ----------------------------------------------

    def _obj(self):
        """The handle of the currently selected schematic."""
        index = self.app.si_index
        names = list(self._schematics)
        if 0 <= index < len(names):
            return self._schematics[names[index]]
        return None

    def _position(self):
        def integer(text):
            return int(text) if text.strip() else 0
        return [
            integer(str(self.app.si_x)),
            integer(str(self.app.si_y)),
        ]

    def _rotation(self):
        return int(str(self.app.si_rotation)) if str(self.app.si_rotation).strip() else 0

    def list_clicked(self):
        obj = self._obj()
        if obj is None:
            return
        self.app.call("SymbolInstance/list", {"obj": obj}, self._on_result)

    def add_clicked(self):
        obj = self._obj()
        if obj is None:
            return
        params = {
            "obj": obj,
            "component": str(self.app.si_component).strip(),
            "representation": str(self.app.si_representation).strip(),
            "gate": str(self.app.si_gate).strip(),
            "position": self._position(),
            "rotation": self._rotation(),
            "mirror": bool(self.app.si_mirror),
            "attributes": [],
        }
        device = str(self.app.si_device).strip()
        if device:
            params["device"] = device
            mpn = str(self.app.si_mpn).strip()
            manufacturer = str(self.app.si_manufacturer).strip()
            if mpn or manufacturer:
                params["part"] = {"mpn": mpn, "manufacturer": manufacturer}
        self.app.call("SymbolInstance/add", params, self._on_result)

    def add_gate_clicked(self):
        obj = self._obj()
        if obj is None:
            return
        params = {
            "obj": obj,
            "component": str(self.app.si_component).strip(),
            "gate": str(self.app.si_gate).strip(),
            "position": self._position(),
            "rotation": self._rotation(),
            "mirror": bool(self.app.si_mirror),
        }
        self.app.call("SymbolInstance/add_gate", params, self._on_result)

    def _on_result(self, status, payload):
        # Print the formatted response data 1:1 (like ProjectEditor/get).
        self.app.si_details = fmt_details(payload)
