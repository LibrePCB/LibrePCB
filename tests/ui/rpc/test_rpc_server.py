#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the RPC server (msgpack protocol, async dispatch)

Uses the `u-msgpack-python` package for the MessagePack encoding.
"""

import socket
import time

import umsgpack

TOKEN = "librepcb-dev-token"


# --- A minimal RPC client ---------------------------------------------------


class RpcClient:
    def __init__(self, port, timeout=30):
        deadline = time.time() + timeout
        while True:
            try:
                self._sock = socket.create_connection(("127.0.0.1", port), timeout=10)
                break
            except ConnectionRefusedError:
                if time.time() > deadline:
                    raise
                time.sleep(0.1)
        self._file = self._sock.makefile("rb")

    def close(self):
        self._file.close()
        self._sock.close()

    def send(self, value):
        self._sock.sendall(umsgpack.packb(value))

    def receive(self):
        """Read one message (blocking). Returns None on EOF."""
        if not self._file.peek(1):
            return None
        try:
            return umsgpack.unpack(self._file)
        except umsgpack.InsufficientDataException as e:
            raise AssertionError(f"truncated message: {e}")

    def request(self, msgid, method, params):
        self.send([0, msgid, method, params])
        message = self.receive()
        assert isinstance(message, list), f"not a response: {message!r}"
        assert message[0] == 1, f"not a response: {message!r}"
        assert message[1] == msgid, f"unexpected msgid: {message!r}"
        assert len(message) == 4, f"invalid response: {message!r}"
        return message[2], message[3]  # (error, result)

    def expect_error(self, msgid, method, params, code):
        error, result = self.request(msgid, method, params)
        assert error is not None, f"expected error, got result: {result!r}"
        assert result is None
        assert error["code"] == code, f"unexpected error: {error!r}"
        return error


# --- The tests --------------------------------------------------------------


def test_rpc_server(librepcb):
    """
    Smoke test for the RPC server: handshake, protocol-level methods,
    commands with a real open project, error handling and pipelining.
    """
    librepcb.add_project("Empty Project")
    librepcb.set_project("Empty Project/Empty Project.lpp")
    # Note: The RPC server is enabled by the demo workspace settings file
    # (tests/data/workspaces/Empty Workspace), no env var needed.
    # Let the OS pick a free port which we then tell the application to use.
    sock = socket.socket()
    sock.bind(("127.0.0.1", 0))
    port = sock.getsockname()[1]
    sock.close()
    librepcb.env["LIBREPCB_RPC_PORT"] = str(port)

    with librepcb.open() as app:  # noqa: F841
        rpc = RpcClient(port)
        try:
            # --- Handshake -----------------------------------------------
            error, result = rpc.request(1, "initialize", {"token": TOKEN})
            assert error is None, f"initialize failed: {error!r}"
            assert result["protocolVersion"] == "1"
            assert isinstance(result["appVersion"], str)
            assert isinstance(result["pid"], int)
            assert isinstance(result["workspace"], str)

            # --- Protocol-level methods ----------------------------------
            error, result = rpc.request(2, "ping", None)
            assert error is None
            assert result == {}

            # --- Commands -------------------------------------------------
            # Project.list → exactly the project opened via the CLI. Since
            # the application opens command-line projects asynchronously
            # after startup, poll until it shows up (once the "project
            # opened" event exists, clients don't need to poll anymore).
            projects = []
            deadline = time.time() + 10
            while time.time() < deadline:
                error, result = rpc.request(3, "Project.list", {})
                assert error is None
                assert isinstance(result, dict)
                projects = result["projects"]
                assert isinstance(projects, list)
                if projects:
                    break
                time.sleep(0.1)
            assert len(projects) == 1
            project = projects[0]
            assert project["name"] == "Empty Project"
            # The object handle is opaque: it's taken from the response as-is
            # and passed back unchanged, without inspecting it.
            handle = project["obj"]
            for key in ["uuid", "author", "version", "path", "modified"]:
                assert key in project, f"missing key {key} in {project!r}"

            # Project.get → details with sheets & boards.
            error, result = rpc.request(4, "Project.get", {"obj": handle})
            assert error is None
            assert result["obj"] == handle
            assert isinstance(result["sheets"], list)
            assert isinstance(result["boards"], list)

            # UndoStack.get_status → the project was just opened, so the
            # stack is clean and there's nothing to undo.
            error, result = rpc.request(5, "UndoStack.get_status", {"obj": handle})
            assert error is None
            assert result == {
                "canUndo": False,
                "canRedo": False,
                "undoText": "",
                "redoText": "",
                "clean": True,
            }

            # UndoStack.undo on an empty stack → no-op, still a success.
            error, result = rpc.request(6, "UndoStack.undo", {"obj": handle})
            assert error is None
            assert result["clean"] is True

            # --- Error handling -------------------------------------------
            # Unknown method → -32601, the connection stays open.
            rpc.expect_error(7, "Foo.bar", {}, 22)

            # Invalid envelope (method not a string) → -32600, the
            # connection stays open.
            rpc.send([0, 8, 42, {}])
            message = rpc.receive()
            assert message[0:2] == [1, 8], message
            assert message[2]["code"] == 20, message
            assert message[3] is None, message

            # Unknown project handle → ProjectNotOpen (-32003), with the
            # requested handle as error property.
            rpc.send([0, 9, "UndoStack.get_status", {"obj": "invalid-uuid"}])
            message = rpc.receive()
            assert message[0:2] == [1, 9], message
            assert message[2]["code"] == 101, message
            assert message[2]["message"] == "Project not open", message
            assert message[3] is None, message

            # Params outside the protocol's value domain (u64 > i64::MAX
            # and non-string map keys) → -32602 "invalid params", the
            # connection stays open (unlike a decode error!).
            rpc.send([0, 10, "Project.list", {"key": 0x8000000000000000}])
            message = rpc.receive()
            assert message[0:2] == [1, 10], message
            assert message[2]["code"] == 23, message

            rpc.send([0, 11, "Project.list", {1: "x"}])  # Non-string map key.
            message = rpc.receive()
            assert message[0:2] == [1, 11], message
            assert message[2]["code"] == 23, message

            # A cancel for an unknown msgid is ignored (no response).
            rpc.send([4, 999])
            error, result = rpc.request(13, "ping", None)
            assert error is None
            assert result == {}

            # --- Pipelining -----------------------------------------------
            # Send three requests without waiting for responses; exactly
            # three responses arrive.
            for msgid in (20, 21, 22):
                rpc.send([0, msgid, "ping", None])
            for msgid in (20, 21, 22):
                message = rpc.receive()
                assert message[0:2] == [1, msgid], message
                assert message[2] is None, message
                assert message[3] == {}, message
        finally:
            rpc.close()
