"""MessagePack RPC client for the RPC GUI (see `dev/plans/rpc-server.md`).

The client is deliberately dumb: It sends requests, assigns msgids and
routes each response back to the handler which was registered for it.
Interpretation of the payloads is up to the pages.
"""

import queue
import socket
import threading
import time

import umsgpack

PORT = 19536


def fmt_value(value):
    """Format a msgpack value into one compact line (for the log)."""
    text = _fmt(value)
    return text if len(text) <= 160 else text[:157] + "…"


def fmt_details(value):
    """Format a response payload as readable, multi-line text."""
    return "\n".join(_detail_lines(value, 0)) or "nil"


def _fmt(value):
    if isinstance(value, umsgpack.Ext):
        return _fmt_ext(value)
    if isinstance(value, dict):
        items = ", ".join(f"{key}: {_fmt(item)}" for key, item in value.items())
        return "{" + items + "}"
    if isinstance(value, (list, tuple)):
        return "[" + ", ".join(_fmt(item) for item in value) + "]"
    if value is None:
        return "nil"
    if isinstance(value, str) and len(value) > 32:
        return value[:29] + "…"
    return str(value)


def fmt_uuid(value):
    """Format a uuid ext value into its canonical string form (e.g. for
    displaying it in the UI)."""
    if (
        isinstance(value, umsgpack.Ext)
        and value.type == 2
        and len(value.data) == 16
    ):
        hexstr = value.data.hex()
        return "%s-%s-%s-%s-%s" % (
            hexstr[0:8], hexstr[8:12], hexstr[12:16], hexstr[16:20],
            hexstr[20:32],
        )
    return str(value)


def _fmt_ext(ext):
    # The UUID ext type (see the API specification), rendered as string.
    if ext.type == 2 and len(ext.data) == 16:
        hexstr = ext.data.hex()
        return "'%s-%s-%s-%s-%s'" % (
            hexstr[0:8], hexstr[8:12], hexstr[12:16], hexstr[16:20],
            hexstr[20:32],
        )
    return f"ext {ext.type} <{ext.data.hex()}>"


def _detail_lines(value, indent):
    pad = "  " * indent
    if isinstance(value, dict):
        lines = []
        for key, item in value.items():
            if isinstance(item, (dict, list)) and item:
                lines.append(f"{pad}{key}:")
                lines += _detail_lines(item, indent + 1)
            else:
                lines.append(f"{pad}{key}: {_fmt(item)}")
        return lines
    if isinstance(value, list):
        lines = []
        for item in value:
            if isinstance(item, (dict, list)) and item:
                lines.append(f"{pad}-")
                lines += _detail_lines(item, indent + 1)
            else:
                lines.append(f"{pad}- {_fmt(item)}")
        return lines
    return [f"{pad}{_fmt(value)}"]


class RpcClient:
    def __init__(self):
        self.sock = socket.create_connection(("127.0.0.1", PORT))
        self.file = self.sock.makefile("rb")
        self.msgid = 0
        self.pending_msgid = None  # The initialize request, for cancelling.
        self.handlers = {}  # msgid -> (method, handler | None, sent_at)
        self.received = queue.Queue()
        threading.Thread(target=self.read_loop, daemon=True).start()

    def read_loop(self):
        while True:
            try:
                msg = umsgpack.unpack(self.file)
            except Exception:
                msg = None
            self.received.put((time.time(), msg))
            if msg is None:
                break

    def call(self, method, params, handler=None):
        """Send a request and remember (method, handler) for its response."""
        self.msgid += 1
        msgid = self.msgid
        self.handlers[msgid] = (method, handler, time.time())
        self.sock.sendall(umsgpack.packb([0, msgid, method, params]))
        return msgid

    def cancel(self, msgid):
        self.sock.sendall(umsgpack.packb([4, msgid]))

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass
