"""Controls for the GraphicsExport class (rendering of graphics scenes)."""

import os
import tempfile

import slint


class GraphicsExportPage:
    def __init__(self, app):
        self.app = app
        self._objects = []  # [{"label", "obj"}] renderable scenes
        self._selected = None  # The handle of the selected scene.
        self._rendering_msgid = None  # The pending GraphicsExport/render.
        self._image_path = None  # Temporary file of the rendered image.

    # --- Object management (only on explicit Refresh) -------------------

    def refresh_clicked(self):
        """Rebuild the dropdown with one ProjectEditor/list and one
        ProjectEditor/get per project."""
        self.app.call("ProjectEditor/list", {}, self._on_refresh_list)

    def _on_refresh_list(self, status, payload):
        if status != 0:
            return
        self._objects = []
        self._update_model()
        for project in payload.get("projects", []):
            self.app.call(
                "ProjectEditor/get", {"obj": project["obj"]},
                lambda status, payload, project=project:
                self._on_refresh_details(status, payload, project))

    def _on_refresh_details(self, status, payload, project):
        if status != 0:
            return
        name = project["name"]
        for sheet in payload.get("sheets", []):
            self._objects.append({
                "label": f"{name}: {sheet['name']} (Schematic)",
                "obj": sheet["obj"],
            })
        for board in payload.get("boards", []):
            self._objects.append({
                "label": f"{name}: {board['name']} (Board)",
                "obj": board["obj"],
            })
        self._update_model()

    def _update_model(self):
        self.app.ge_entries = slint.ListModel(
            [entry["label"] for entry in self._objects])
        # Keep the selection if it still exists, otherwise select the
        # first entry (the Render button needs a selection).
        selected = next(
            (entry for entry in self._objects
             if entry["obj"] == self._selected), None)
        if selected is None and self._objects:
            selected = self._objects[0]
        if selected is not None:
            self._selected = selected["obj"]
            self.app.ge_index = self._objects.index(selected)
        else:
            self._selected = None
            self.app.ge_index = -1

    def select(self, value):
        """The dropdown was used to select a scene (by display string)."""
        for entry in self._objects:
            if entry["label"] == value:
                self._selected = entry["obj"]
                return

    # --- Command handlers ----------------------------------------------

    def render_clicked(self):
        if (self._selected is None) or (self._rendering_msgid is not None):
            return
        params = {
            "obj": self._selected,
            "dpi": self.app.ge_dpi,
            "flip": self.app.ge_flip,
            "realistic": self.app.ge_realistic,
        }
        self.app.ge_running = True
        self.app.ge_status = "Rendering..."
        self._rendering_msgid = self.app.call(
            "GraphicsExport/render", params, self._on_render)

    def _on_render(self, status, payload):
        self._rendering_msgid = None
        self.app.ge_running = False
        if status != 0:
            self.app.ge_status = "Render failed."
            return
        data = payload.get("image")
        if not isinstance(data, (bytes, bytearray)):
            self.app.ge_status = "No image in response."
            return
        try:
            self._set_image(bytes(data))
            self.app.ge_status = f"{len(data)} bytes"
        except (OSError, RuntimeError) as e:
            self.app.ge_status = f"Failed to load image: {e}"

    def _set_image(self, data):
        # Keep the file until the next render: Some platforms don't allow
        # removing a file while the image loader still has it open.
        if self._image_path:
            try:
                os.unlink(self._image_path)
            except OSError:
                pass
        fd, path = tempfile.mkstemp(suffix=".png")
        with os.fdopen(fd, "wb") as f:
            f.write(data)
        self._image_path = path
        self.app.ge_image = slint.Image.load_from_path(path)
