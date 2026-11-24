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
        return message[2], message[3]  # (status, payload)

    def expect_error(self, msgid, method, params, code):
        status, payload = self.request(msgid, method, params)
        assert status != 0, f"expected error, got result: {payload!r}"
        assert isinstance(payload, dict) and "message" in payload, payload
        assert status == code, f"unexpected error: {status}, {payload!r}"
        return payload


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
            assert message[2] == 0, message  # Status: success.
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
                status, result = rpc2.request(2, "initialize", {"token": token})
                assert status == 0, f"initialize failed: {result!r}"
                assert result["format"] == "1.0", result
                assert result["server"] == "librepcb", result
                assert result["token"] == token, result
            finally:
                rpc2.close()

            # A token which was not issued by this application instance is
            # rejected, and the connection stays unauthenticated.
            rpc3 = RpcClient(PORT)
            try:
                rpc3.expect_error(1, "initialize", {"token": "invalid"}, 33)
                status, payload = rpc3.request(2, "ping", None)
                assert status == 30, payload
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
                assert message[2] == 33, message  # Status: the error code.
                assert message[3]["message"] == "Access denied", message
            finally:
                rpc4.close()

            # --- Protocol-level methods ----------------------------------
            status, result = rpc.request(2, "ping", None)
            assert status == 0
            assert result is None

            # --- Commands -------------------------------------------------
            # ProjectEditor.list → exactly the project opened via the CLI. Since
            # the application opens command-line projects asynchronously
            # after startup, poll until it shows up (once the "project
            # opened" event exists, clients don't need to poll anymore).
            projects = []
            deadline = time.time() + 10
            while time.time() < deadline:
                status, result = rpc.request(3, "ProjectEditor.list", {})
                assert status == 0
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
            for key in ["path", "dirty"]:
                assert key in project, f"missing key {key} in {project!r}"

            # ProjectEditor.get → details with sheets & boards.
            status, result = rpc.request(4, "ProjectEditor.get", {"obj": handle})
            assert status == 0
            assert isinstance(result["sheets"], list)
            assert isinstance(result["boards"], list)

            # UndoStack.get → the project was just opened, so the stack is
            # clean and there's nothing to undo/redo.
            status, result = rpc.request(5, "UndoStack.get", {"obj": handle})
            assert status == 0
            assert result["clean"] is True, result
            assert result["prev"] is None and result["next"] is None, result
            assert isinstance(result["id"], int), result

            # UndoStack.undo on an empty stack → UnexpectedState.
            rpc.expect_error(6, "UndoStack.undo", {"obj": handle}, 50)

            # --- Error handling -------------------------------------------
            # Unknown method → MethodNotFound (31), the connection
            # stays open.
            rpc.expect_error(7, "Foo.bar", {}, 31)

            # Invalid envelope (method not a string) → InvalidRequest
            # (20), the connection stays open.
            rpc.send([0, 8, 42, {}])
            message = rpc.receive()
            assert message[0:2] == [1, 8], message
            assert message[2] == 20, message  # Status: the error code.
            assert message[3]["message"] == "Invalid request", message

            # Unknown project handle → InvalidHandle (41).
            rpc.send([0, 9, "UndoStack.get", {"obj": "invalid-uuid"}])
            message = rpc.receive()
            assert message[0:2] == [1, 9], message
            assert message[2] == 41, message  # Status: the error code.
            assert message[3]["message"] == "Invalid object handle", message

            # Large u64 values are valid integers (native msgpack):
            rpc.send([0, 10, "ProjectEditor.list", {"key": 0x8000000000000000}])
            message = rpc.receive()
            assert message[0:2] == [1, 10], message
            assert message[2] == 0, message  # Status: success.
            assert isinstance(message[3], dict), message

            # Params outside the protocol's value domain (non-string map
            # keys) → InvalidArgs (40), the connection stays open (unlike
            # a decode error!).
            rpc.send([0, 11, "ProjectEditor.list", {1: "x"}])  # Non-string map key.
            message = rpc.receive()
            assert message[0:2] == [1, 11], message
            assert message[2] == 40, message  # Status: the error code.

            # A cancel for an unknown msgid is ignored (no response).
            rpc.send([4, 999])
            status, result = rpc.request(13, "ping", None)
            assert status == 0
            assert result is None

            # --- Pipelining -----------------------------------------------
            # Send three requests without waiting for responses; exactly
            # three responses arrive.
            for msgid in (20, 21, 22):
                rpc.send([0, msgid, "ping", None])
            for msgid in (20, 21, 22):
                message = rpc.receive()
                assert message[0:2] == [1, msgid], message
                assert message[2] == 0, message  # Status: success.
                assert message[3] is None, message
        finally:
            rpc.close()
