"""Controls for the UndoStack class."""

from client import fmt_details


class UndoStackPage:
    def __init__(self, app):
        self.app = app

    def get_clicked(self):
        self._call("UndoStack.get")

    def undo_clicked(self):
        self._call("UndoStack.undo")

    def redo_clicked(self):
        self._call("UndoStack.redo")

    def _call(self, method):
        # The handle field is auto-filled when acting on a project row, but
        # can be edited freely (e.g. to test stale or invalid handles).
        text = self.app.undo_handle.strip()
        if not text:
            return
        handle = int(text) if text.isdigit() else text
        self.app.call(method, {"obj": handle}, self._on_details)

    def _on_details(self, error, result):
        if error is None:
            self.app.undo_details = fmt_details(result)
