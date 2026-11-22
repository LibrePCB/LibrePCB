#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the RPC server (msgpack protocol, async dispatch)

Uses the `u-msgpack-python` package for the MessagePack encoding.
"""

import socket
import time

import umsgpack

# The default RPC server port (the letters "LP" interpreted as a
# hexadecimal number: 0x4C50 = 19536). Keep in sync with DEFAULT_PORT in
# `rust-core/src/rpc_server.rs`.
PORT = 19536


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


def test_rpc_server(librepcb, helpers):
    """
    Smoke test for the RPC server: handshake (interactive authentication
    and token issuance), protocol-level methods, commands with a real open
    project, error handling and pipelining.
    """
    librepcb.add_project("Empty Project")
    librepcb.set_project("Empty Project/Empty Project.lpp")
    # Note: The RPC server is enabled by the demo workspace settings file
    # (tests/data/workspaces/Empty Workspace).

    with librepcb.open() as app:  # noqa: F841
        rpc = RpcClient(PORT)
        try:
            # --- Handshake -----------------------------------------------
            # The first connection has no token yet, so the user has to
            # accept the connection interactively: The response is only
            # sent after clicking the notification button.
            rpc.send([0, 1, "initialize", {"name": "RPC Test"}])
            notification = helpers.wait_for_notification(
                app, "RPC Connection Request"
            )
            notification.trigger()
            message = rpc.receive()
            assert message[0:2] == [1, 1], message
            assert message[2] is None, message
            result = message[3]
            assert result["format"] == "1.0", result
            assert result["server"] == "librepcb", result
            assert isinstance(result["version"], str), result
            # Verify that we're connected to the expected application
            # instance (the port fallback could have chosen a different
            # port if a stale instance was still listening).
            assert result["workspace"] == librepcb.workspace_path.replace(
                "\\", "/"
            ), result
            # A token was issued, to be used for future connections.
            token = result["token"]
            assert isinstance(token, str) and len(token) > 10, result

            # A second connection can authenticate with the issued token,
            # without any user interaction.
            rpc2 = RpcClient(PORT)
            try:
                error, result = rpc2.request(2, "initialize", {"token": token})
                assert error is None, f"initialize failed: {error!r}"
                assert result["format"] == "1.0", result
                assert result["server"] == "librepcb", result
                assert result["token"] == token, result
            finally:
                rpc2.close()

            # A token which was not issued by this application instance is
            # rejected, and the connection stays unauthenticated.
            rpc3 = RpcClient(PORT)
            try:
                rpc3.expect_error(1, "initialize", {"token": "invalid"}, 31)
                error, result = rpc3.request(2, "ping", None)
                assert error is not None and error["code"] == 30, error
            finally:
                rpc3.close()

            # Cancelling a pending connection request denies access.
            rpc4 = RpcClient(PORT)
            try:
                rpc4.send([0, 1, "initialize", {"name": "RPC Test"}])
                helpers.wait_for_notification(
                    app, "RPC Connection Request"
                )
                rpc4.send([4, 1])
                message = rpc4.receive()
                assert message[0:2] == [1, 1], message
                assert message[2]["code"] == 31, message
                assert message[2]["message"] == "Access denied", message
            finally:
                rpc4.close()

            # --- Protocol-level methods ----------------------------------
            error, result = rpc.request(2, "ping", None)
            assert error is None
            assert result is None

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
            # Unknown method → MethodNotFound (32), the connection
            # stays open.
            rpc.expect_error(7, "Foo.bar", {}, 32)

            # Invalid envelope (method not a string) → InvalidRequest
            # (20), the connection stays open.
            rpc.send([0, 8, 42, {}])
            message = rpc.receive()
            assert message[0:2] == [1, 8], message
            assert message[2]["code"] == 20, message
            assert message[3] is None, message

            # Unknown project handle → InvalidHandle (35).
            rpc.send([0, 9, "UndoStack.get_status", {"obj": "invalid-uuid"}])
            message = rpc.receive()
            assert message[0:2] == [1, 9], message
            assert message[2]["code"] == 35, message
            assert message[2]["message"] == "Invalid object handle", message
            assert message[3] is None, message

            # Params outside the protocol's value domain (u64 > i64::MAX
            # and non-string map keys) → InvalidArgs (34), the connection
            # stays open (unlike a decode error!).
            rpc.send([0, 10, "Project.list", {"key": 0x8000000000000000}])
            message = rpc.receive()
            assert message[0:2] == [1, 10], message
            assert message[2]["code"] == 34, message

            rpc.send([0, 11, "Project.list", {1: "x"}])  # Non-string map key.
            message = rpc.receive()
            assert message[0:2] == [1, 11], message
            assert message[2]["code"] == 34, message

            # A cancel for an unknown msgid is ignored (no response).
            rpc.send([4, 999])
            error, result = rpc.request(13, "ping", None)
            assert error is None
            assert result is None

            # --- Pipelining -----------------------------------------------
            # Send three requests without waiting for responses; exactly
            # three responses arrive.
            for msgid in (20, 21, 22):
                rpc.send([0, msgid, "ping", None])
            for msgid in (20, 21, 22):
                message = rpc.receive()
                assert message[0:2] == [1, msgid], message
                assert message[2] is None, message
                assert message[3] is None, message
        finally:
            rpc.close()
