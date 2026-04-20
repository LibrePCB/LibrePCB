#!/usr/bin/env python3
"""
Quick-and-dirty MCP server for controlling a running LibrePCB window.

LibrePCB must be started with LIBREPCB_ENABLE_TEST_ADAPTER=1 so that
the TCP command server on localhost:19999 is active.

MCP tools exposed:
  - switch_to_about_panel   : shows the About panel in the sidebar
  - switch_to_tab           : switches to a tab by index in a window section
"""

import os
import sys
import json
import socket
import threading

LIBREPCB_HOST = os.environ.get("LIBREPCB_HOST", "172.17.0.1")
LIBREPCB_PORT = int(os.environ.get("LIBREPCB_PORT", "19999"))


# ---------------------------------------------------------------------------
# Helpers to talk to LibrePCB
# ---------------------------------------------------------------------------

def send_command(cmd: dict) -> dict:
    """Send a JSON command to the LibrePCB TCP server and return the response."""
    with socket.create_connection((LIBREPCB_HOST, LIBREPCB_PORT), timeout=5) as s:
        s.sendall((json.dumps(cmd) + "\n").encode())
        data = b""
        while not data.endswith(b"\n"):
            chunk = s.recv(4096)
            if not chunk:
                break
            data += chunk
    return json.loads(data.strip())


# ---------------------------------------------------------------------------
# Minimal MCP server (JSON-RPC 2.0 over stdio)
# ---------------------------------------------------------------------------

TOOLS = [
    {
        "name": "switch_to_about_panel",
        "description": "Switch the LibrePCB sidebar to the About panel.",
        "inputSchema": {
            "type": "object",
            "properties": {},
            "required": [],
        },
    },
    {
        "name": "switch_to_tab",
        "description": (
            "Switch to a tab by index in a LibrePCB window section. "
            "Tab 0 is usually the Home tab."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "index": {
                    "type": "integer",
                    "description": "Zero-based tab index to switch to.",
                },
                "section": {
                    "type": "integer",
                    "description": "Zero-based window section index (default 0).",
                },
            },
            "required": ["index"],
        },
    },
]


def handle_tool_call(name: str, arguments: dict) -> str:
    if name == "switch_to_about_panel":
        resp = send_command({"command": "show-about-panel"})
        if resp.get("ok"):
            return "About panel is now shown."
        return f"Error: {resp.get('error', 'unknown')}"

    if name == "switch_to_tab":
        index = arguments.get("index", 0)
        section = arguments.get("section", 0)
        resp = send_command({"command": "switch-tab", "section": section, "index": index})
        if resp.get("ok"):
            return f"Switched to tab {index} in section {section}."
        return f"Error: {resp.get('error', 'unknown')}"

    return f"Unknown tool: {name}"


def make_response(req_id, result):
    return {"jsonrpc": "2.0", "id": req_id, "result": result}


def make_error(req_id, code, message):
    return {"jsonrpc": "2.0", "id": req_id, "error": {"code": code, "message": message}}


def handle_request(req: dict) -> dict | None:
    method = req.get("method", "")
    req_id = req.get("id")

    if method == "initialize":
        return make_response(req_id, {
            "protocolVersion": "2024-11-05",
            "serverInfo": {"name": "librepcb-mcp", "version": "0.1.0"},
            "capabilities": {"tools": {}},
        })

    if method == "notifications/initialized":
        return None  # notification, no response

    if method == "tools/list":
        return make_response(req_id, {"tools": TOOLS})

    if method == "tools/call":
        params = req.get("params", {})
        tool_name = params.get("name", "")
        arguments = params.get("arguments", {})
        try:
            text = handle_tool_call(tool_name, arguments)
            return make_response(req_id, {
                "content": [{"type": "text", "text": text}],
            })
        except Exception as exc:
            return make_response(req_id, {
                "content": [{"type": "text", "text": f"Error: {exc}"}],
                "isError": True,
            })

    if method == "ping":
        return make_response(req_id, {})

    # Unknown method
    if req_id is not None:
        return make_error(req_id, -32601, f"Method not found: {method}")
    return None


def main():
    stdin = sys.stdin.buffer
    stdout = sys.stdout.buffer

    while True:
        line = stdin.readline()
        if not line:
            break
        line = line.strip()
        if not line:
            continue
        try:
            req = json.loads(line)
        except json.JSONDecodeError as e:
            resp = make_error(None, -32700, f"Parse error: {e}")
            stdout.write((json.dumps(resp) + "\n").encode())
            stdout.flush()
            continue

        resp = handle_request(req)
        if resp is not None:
            stdout.write((json.dumps(resp) + "\n").encode())
            stdout.flush()


if __name__ == "__main__":
    main()
