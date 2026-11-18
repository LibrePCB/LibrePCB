#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the RPC server
"""

import json
import socket

TOKEN = "librepcb-dev-token"


class RpcConnection(object):
    """A minimal JSON-RPC client connected to the RPC server."""

    def __init__(self, port):
        self._socket = socket.create_connection(("127.0.0.1", port), timeout=10)
        self._socket_file = self._socket.makefile("r")
        self._id = 0

    def close(self):
        self._socket_file.close()
        self._socket.close()

    def send(self, method, params):
        self._id += 1
        request = {"jsonrpc": "2.0", "id": self._id, "method": method,
                   "params": params}
        self._socket.sendall(
            (json.dumps(request) + "\n").encode("utf-8")
        )
        return self._id

    def receive(self):
        line = self._socket_file.readline()
        return json.loads(line)

    def request(self, method, params):
        request_id = self.send(method, params)
        response = self.receive()
        assert response["id"] == request_id
        return response


def _find_free_port():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(("127.0.0.1", 0))
    port = sock.getsockname()[1]
    sock.close()
    return port


def test_rpc_server(librepcb):
    """
    Smoke test for the RPC server (handshake + a couple of commands)
    """
    port = _find_free_port()
    librepcb.env["LIBREPCB_ENABLE_RPC_SERVER"] = "1"
    librepcb.env["LIBREPCB_RPC_PORT"] = str(port)

    with librepcb.open() as app:
        rpc = RpcConnection(port)
        try:
            # Handshake with the hardcoded development token.
            response = rpc.request("initialize", {"token": TOKEN})
            result = response["result"]
            assert result["protocolVersion"] == "1"
            assert isinstance(result["appVersion"], str)
            assert isinstance(result["pid"], int)
            assert "workspace" in result

            # Protocol-level ping.
            assert rpc.request("ping", {})["result"] == {}

            # Commands.
            assert rpc.request("project/list", {})["result"] == {
                "projects": []
            }
            response = rpc.request("undostack/getStatus", {
                "project": "invalid-uuid",
            })
            assert response["error"]["code"] == -32003

            # Unknown methods are rejected, but the connection stays open.
            response = rpc.request("foo/bar", {})
            assert response["error"]["code"] == -32601
            assert rpc.request("ping", {})["result"] == {}

            # Malformed JSON is rejected, but the connection stays open.
            rpc._socket.sendall(b"this is not json\n")  # noqa: SLF001
            response = rpc.receive()
            assert response["error"]["code"] == -32700
            assert rpc.request("ping", {})["result"] == {}

            # The API schema is served by the standard discovery method.
            schema = rpc.request("rpc.discover", {})["result"]
            assert schema["openrpc"] == "1.2.6"
            methods = {m["name"] for m in schema["methods"]}
            assert "project/list" in methods
        finally:
            rpc.close()

        # A connection with a wrong token gets disconnected immediately.
        rpc = RpcConnection(port)
        try:
            response = rpc.request("initialize", {"token": "wrong"})
            assert response["error"]["code"] == -32001
            assert rpc._socket_file.readline() == ""  # noqa: SLF001
        finally:
            rpc.close()
