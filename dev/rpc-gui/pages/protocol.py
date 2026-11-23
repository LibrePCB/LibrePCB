"""Controls for the protocol-level methods (class "Protocol")."""


class ProtocolPage:
    def __init__(self, app):
        self.app = app

    def ping_clicked(self):
        self.app.call("ping", {})
