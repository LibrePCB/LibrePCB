#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test the RPC server (msgpack protocol, async dispatch)
"""

import socket
import struct
import time

TOKEN = "librepcb-dev-token"


# --- Minimal MessagePack codec -------------------------------------------
#
# Intentionally implemented inline instead of depending on the `msgpack`
# package: This way the test suite needs no additional dependency, and it
# proves that the protocol is implementable from the specification alone.
# Only the subset needed by the RPC protocol is supported.


class _Incomplete(Exception):
    """Raised when a buffer does not contain a complete msgpack value yet."""


def _pack(value, out):
    """Append the msgpack encoding of a value to a bytearray."""
    if value is None:
        out.append(0xC0)
    elif value is True:
        out.append(0xC3)
    elif value is False:
        out.append(0xC2)
    elif isinstance(value, int):
        if 0 <= value < 0x80:
            out.append(value)
        elif -0x20 <= value < 0:
            out.append(0xE0 + 0x20 + value)
        elif 0 <= value < 0x100:
            out.append(0xCC)
            out.append(value)
        elif 0 <= value < 0x10000:
            out.append(0xCD)
            out.extend(value.to_bytes(2, "big"))
        elif 0 <= value < 0x100000000:
            out.append(0xCE)
            out.extend(value.to_bytes(4, "big"))
        elif 0 <= value < 0x10000000000000000:
            out.append(0xCF)
            out.extend(value.to_bytes(8, "big"))
        elif -0x80 <= value < 0x80:
            out.append(0xD0)
            out.extend(value.to_bytes(1, "big", signed=True))
        elif -0x8000 <= value < 0x8000:
            out.append(0xD1)
            out.extend(value.to_bytes(2, "big", signed=True))
        elif -0x80000000 <= value < 0x80000000:
            out.append(0xD2)
            out.extend(value.to_bytes(4, "big", signed=True))
        else:
            out.append(0xD3)
            out.extend(value.to_bytes(8, "big", signed=True))
    elif isinstance(value, float):
        out.append(0xCB)
        out.extend(struct.pack(">d", value))
    elif isinstance(value, str):
        _pack_bytes(out, value.encode("utf-8"), 0xA0, 0x1F, 0xD9, 0xDA, 0xDB)
    elif isinstance(value, bytes):
        _pack_bytes(out, value, 0xC4, 0xFF, 0xC4, 0xC5, 0xC6)
    elif isinstance(value, list):
        if len(value) < 0x10:
            out.append(0x90 | len(value))
        elif len(value) < 0x10000:
            out.append(0xDC)
            out.extend(len(value).to_bytes(2, "big"))
        else:
            out.append(0xDD)
            out.extend(len(value).to_bytes(4, "big"))
        for item in value:
            _pack(item, out)
    elif isinstance(value, dict):
        if len(value) < 0x10:
            out.append(0x80 | len(value))
        elif len(value) < 0x10000:
            out.append(0xDE)
            out.extend(len(value).to_bytes(2, "big"))
        else:
            out.append(0xDF)
            out.extend(len(value).to_bytes(4, "big"))
        for key, item in value.items():
            _pack(key, out)
            _pack(item, out)
    else:
        raise NotImplementedError(f"cannot pack {type(value)}")


def _pack_bytes(out, data, fix_marker, fix_mask, m8, m16, m32):
    """Pack a str or bytes value (fixstr/strN or binN)."""
    if isinstance(data, str):
        data = data.encode("utf-8")
    if len(data) <= fix_mask:
        out.append(fix_marker | len(data))
        out.extend(data)
    elif len(data) < 0x100:
        out.append(m8)
        out.append(len(data))
        out.extend(data)
    elif len(data) < 0x10000:
        out.append(m16)
        out.extend(len(data).to_bytes(2, "big"))
        out.extend(data)
    else:
        out.append(m32)
        out.extend(len(data).to_bytes(4, "big"))
        out.extend(data)


def _unpack(buf, pos):
    """Decode one msgpack value, returning (value, new_pos)."""
    if pos >= len(buf):
        raise _Incomplete()
    marker = buf[pos]
    pos += 1
    if marker <= 0x7F:  # positive fixint
        return marker, pos
    if marker >= 0xE0:  # negative fixint
        return marker - 0x100, pos
    if marker <= 0x8F:  # fixmap
        return _unpack_map(buf, pos, marker & 0x0F)
    if marker <= 0x9F:  # fixarray
        return _unpack_array(buf, pos, marker & 0x0F)
    if marker <= 0xBF:  # fixstr
        return _unpack_str(buf, pos, marker & 0x1F)
    if marker == 0xC0:
        return None, pos
    if marker == 0xC2:
        return False, pos
    if marker == 0xC3:
        return True, pos
    if marker == 0xC4:  # bin8
        return _unpack_bin(buf, pos, _unpack_u8(buf, pos))
    if marker == 0xC5:  # bin16
        return _unpack_bin(buf, pos + 2, _unpack_u16(buf, pos))
    if marker == 0xC6:  # bin32
        return _unpack_bin(buf, pos + 4, _unpack_u32(buf, pos))
    if marker == 0xCA:  # float32
        data, pos = _unpack_raw(buf, pos, 4)
        return struct.unpack(">f", data)[0], pos
    if marker == 0xCB:  # float64
        data, pos = _unpack_raw(buf, pos, 8)
        return struct.unpack(">d", data)[0], pos
    if marker == 0xCC:
        return _unpack_u8(buf, pos), pos + 1
    if marker == 0xCD:
        return _unpack_u16(buf, pos), pos + 2
    if marker == 0xCE:
        return _unpack_u32(buf, pos), pos + 4
    if marker == 0xCF:
        return _unpack_u64(buf, pos), pos + 8
    if marker == 0xD0:
        data, _ = _unpack_raw(buf, pos, 1)
        return struct.unpack(">b", data)[0], pos + 1
    if marker == 0xD1:
        data, _ = _unpack_raw(buf, pos, 2)
        return struct.unpack(">h", data)[0], pos + 2
    if marker == 0xD2:
        data, _ = _unpack_raw(buf, pos, 4)
        return struct.unpack(">i", data)[0], pos + 4
    if marker == 0xD3:
        data, _ = _unpack_raw(buf, pos, 8)
        return struct.unpack(">q", data)[0], pos + 8
    if marker == 0xD9:
        return _unpack_str(buf, pos + 1, _unpack_u8(buf, pos))
    if marker == 0xDA:
        return _unpack_str(buf, pos + 2, _unpack_u16(buf, pos))
    if marker == 0xDB:
        return _unpack_str(buf, pos + 4, _unpack_u32(buf, pos))
    if marker == 0xDC:
        return _unpack_array(buf, pos + 2, _unpack_u16(buf, pos))
    if marker == 0xDD:
        return _unpack_array(buf, pos + 4, _unpack_u32(buf, pos))
    if marker == 0xDE:
        return _unpack_map(buf, pos + 2, _unpack_u16(buf, pos))
    if marker == 0xDF:
        return _unpack_map(buf, pos + 4, _unpack_u32(buf, pos))
    raise ValueError(f"unsupported msgpack marker 0x{marker:02X}")


def _unpack_raw(buf, pos, count):
    if pos + count > len(buf):
        raise _Incomplete()
    return buf[pos : pos + count], pos + count


def _unpack_u8(buf, pos):
    if pos + 1 > len(buf):
        raise _Incomplete()
    return buf[pos]


def _unpack_u16(buf, pos):
    if pos + 2 > len(buf):
        raise _Incomplete()
    return int.from_bytes(buf[pos : pos + 2], "big")


def _unpack_u32(buf, pos):
    if pos + 4 > len(buf):
        raise _Incomplete()
    return int.from_bytes(buf[pos : pos + 4], "big")


def _unpack_u64(buf, pos):
    if pos + 8 > len(buf):
        raise _Incomplete()
    return int.from_bytes(buf[pos : pos + 8], "big")


def _unpack_str(buf, pos, length):
    data, pos = _unpack_raw(buf, pos, length)
    return data.decode("utf-8"), pos


def _unpack_bin(buf, pos, length):
    data, pos = _unpack_raw(buf, pos, length)
    return data, pos


def _unpack_array(buf, pos, count):
    items = []
    for _ in range(count):
        value, pos = _unpack(buf, pos)
        items.append(value)
    return items, pos


def _unpack_map(buf, pos, count):
    result = {}
    for _ in range(count):
        key, pos = _unpack(buf, pos)
        value, pos = _unpack(buf, pos)
        result[key] = value
    return result, pos


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
        self._buf = b""

    def close(self):
        self._sock.close()

    def send(self, value):
        out = bytearray()
        _pack(value, out)
        self._sock.sendall(bytes(out))

    def receive(self):
        """Read one message (blocking). Returns None on EOF."""
        while True:
            try:
                value, pos = _unpack(self._buf, 0)
                self._buf = self._buf[pos:]
                return value
            except _Incomplete:
                chunk = self._sock.recv(4096)
                if not chunk:
                    return None
                self._buf += chunk

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
    librepcb.env["LIBREPCB_ENABLE_RPC_SERVER"] = "1"
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
            assert isinstance(project["uuid"], str)
            for key in ["author", "version", "path", "modified"]:
                assert key in project, f"missing key {key} in {project!r}"
            uuid = project["uuid"]

            # Project.get → details with sheets & boards.
            error, result = rpc.request(4, "Project.get", {"obj": uuid})
            assert error is None
            assert result["uuid"] == uuid
            assert isinstance(result["sheets"], list)
            assert isinstance(result["boards"], list)

            # UndoStack.get_status → the project was just opened, so the
            # stack is clean and there's nothing to undo.
            error, result = rpc.request(5, "UndoStack.get_status", {"obj": uuid})
            assert error is None
            assert result == {
                "canUndo": False,
                "canRedo": False,
                "undoText": "",
                "redoText": "",
                "clean": True,
            }

            # UndoStack.undo on an empty stack → no-op, still a success.
            error, result = rpc.request(6, "UndoStack.undo", {"obj": uuid})
            assert error is None
            assert result["clean"] is True

            # --- Error handling -------------------------------------------
            # Unknown method → -32601, the connection stays open.
            rpc.expect_error(7, "Foo.bar", {}, -32601)

            # Invalid envelope (method not a string) → -32600, the
            # connection stays open.
            rpc.send([0, 8, 42, {}])
            message = rpc.receive()
            assert message[0:2] == [1, 8], message
            assert message[2]["code"] == -32600, message
            assert message[3] is None, message

            # Unknown project handle → ProjectNotOpen (-32003), with the
            # requested handle as error property.
            rpc.send([0, 9, "UndoStack.get_status", {"obj": "invalid-uuid"}])
            message = rpc.receive()
            assert message[0:2] == [1, 9], message
            assert message[2]["code"] == -32003, message
            assert message[2]["project"] == "invalid-uuid", message
            assert message[3] is None, message

            # Params outside the protocol's value domain (u64 > i64::MAX
            # and non-string map keys) → -32602 "invalid params", the
            # connection stays open (unlike a decode error!).
            rpc.send([0, 10, "Project.list", {"key": 0x8000000000000000}])
            message = rpc.receive()
            assert message[0:2] == [1, 10], message
            assert message[2]["code"] == -32602, message

            rpc.send([0, 11, "Project.list", {1: "x"}])  # Non-string map key.
            message = rpc.receive()
            assert message[0:2] == [1, 11], message
            assert message[2]["code"] == -32602, message

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
