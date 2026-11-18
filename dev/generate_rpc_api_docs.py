#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""Generate the Doxygen documentation page for the LibrePCB RPC API.

This script generates `dev/doxygen/pages/rpc_api.md` from the OpenRPC
document `share/librepcb/rpc/api.json` (the single source of truth for the
RPC API). The generated file is committed to the repository, so building
the documentation does not require Python.

Usage:

    python3 dev/generate_rpc_api_docs.py           # Generate the docs
    python3 dev/generate_rpc_api_docs.py --check   # Only check if the
                                                   # committed docs are
                                                   # up to date (for CI)

Attention: Do not modify the generated Markdown file manually, modify the
           OpenRPC document instead and re-run this script!
"""

import argparse
import json
import re
import sys
from pathlib import Path

REPO_DIR = Path(__file__).resolve().parent.parent
API_FILE = REPO_DIR / "share" / "librepcb" / "rpc" / "api.json"
DOCS_FILE = REPO_DIR / "dev" / "doxygen" / "pages" / "rpc_api.md"

# Static introduction about the transport protocol. The OpenRPC document
# only contains the dispatched commands, since the protocol-level methods
# (framing, handshake, ping, rpc.discover) are implemented in Rust and not
# part of the command registry.
PROTOCOL_INTRO = """\
# Protocol {#doc_rpc_api_protocol}

The RPC server listens on a local TCP socket (`127.0.0.1`, port assigned by
the operating system) and accepts an unlimited number of connections. The
wire protocol is [JSON-RPC 2.0](https://www.jsonrpc.org/specification), with
exactly one message per line (newline-delimited JSON). This makes the server
easy to debug with tools like `nc 127.0.0.1 <port>`.

## Handshake {#doc_rpc_api_protocol_handshake}

The first message of a connection must be an `initialize` request carrying
the authentication token. Any other first message (or a wrong token) leads
to an error response and an immediate disconnect. Also a previously
authenticated connection is disconnected if a second `initialize` request
contains a wrong token.

~~~
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"token":"<token>"}}
~~~

The success result contains the protocol version and some application
information:

~~~
{"jsonrpc":"2.0","id":1,"result":{"appVersion":"1.3.0","pid":1234,\\
"protocolVersion":"1","workspace":"~/LibrePCB-Workspace"}}
~~~

## Protocol-Level Methods {#doc_rpc_api_protocol_methods}

These methods are handled by the server itself, i.e. they are not
dispatched to the application:

| Method         | Description                                                      |
|----------------|------------------------------------------------------------------|
| `initialize`   | Handshake & authentication (see above).                          |
| `ping`         | Returns an empty result, useful to test a connection.            |
| `rpc.discover` | Returns the OpenRPC API schema (the same document as this page).|

Any other method is dispatched to the application, i.e. it must be listed in
the commands section below. Unknown methods are answered with error
`-32601`.

## Error Codes {#doc_rpc_api_protocol_errors}

| Code   | Meaning                                                  |
|--------|----------------------------------------------------------|
| -32700 | Parse error (the message is not valid JSON).             |
| -32600 | The message is not a valid JSON-RPC 2.0 request.          |
| -32601 | The method does not exist.                               |
| -32602 | Invalid method parameter(s).                             |
| -32603 | Internal error (e.g. the command handler panicked).      |
| -32001 | Not authenticated (invalid or missing token).            |
| -32002 | Command failed (e.g. the command threw an exception).     |
| -32003 | The requested project is not open.                       |
| -32004 | The server is shutting down.                              |

## Request Semantics {#doc_rpc_api_protocol_semantics}

Requests on a single connection are processed strictly sequentially, i.e.
the server does not process the next request of a connection before the
current one was fully dispatched. Within the application, all commands are
executed on the GUI thread, so commands from multiple connections are
automatically serialized. Clients which want to run commands in parallel
need to open multiple connections.

Note that the server is work in progress: The token is currently hardcoded
(not random per instance) and the port is not discoverable yet, see the
notes in `dev/plans/rpc-server.md`."""


def anchor_for(method_name):
    """Convert a method name to a Doxygen-compatible anchor."""
    return "doc_rpc_api_" + re.sub(r"[^a-zA-Z0-9]", "_", method_name)


def schema_type(schema):
    """Flatten a JSON schema to a simple type name."""
    if not isinstance(schema, dict):
        return "unknown"
    if "enum" in schema:
        return " / ".join(str(x) for x in schema["enum"])
    if "properties" in schema:
        return "object"
    return schema.get("type", "unknown")


def schema_description(schema):
    """Get the description of a JSON schema."""
    if not isinstance(schema, dict):
        return ""
    return schema.get("description", "")


def render_params_table(method):
    """Render the parameters of a method as a Markdown table."""
    params = method.get("params", [])
    if not params:
        return "None.\n"
    lines = ["| Name | Type | Required | Description |", "|------|------|----------|-------------|"]
    for param in params:
        name = param.get("name", "?")
        typ = schema_type(param.get("schema"))
        required = "yes" if param.get("required") else "no"
        description = param.get("description", schema_description(param.get("schema")))
        lines.append(f"| `{name}` | {typ} | {required} | {description} |")
    return "\n".join(lines) + "\n"


def render_result_section(method):
    """Render the result of a method as a Markdown table + description."""
    result = method.get("result", {})
    if not isinstance(result, dict):
        return ""
    lines = []
    if "description" in result:
        lines.append(result["description"])
        lines.append("")
    schema = result.get("schema", {})
    properties = schema.get("properties") if isinstance(schema, dict) else None
    if isinstance(properties, dict) and properties:
        lines.append("| Key | Type | Description |")
        lines.append("|-----|------|-------------|")
        for key, subschema in properties.items():
            description = schema_description(subschema)
            lines.append(f"| `{key}` | {schema_type(subschema)} | {description} |")
        lines.append("")
    return "\n".join(lines)


def render_errors_table(method):
    """Render the errors of a method as a Markdown table."""
    errors = method.get("errors", [])
    if not errors:
        return "None specific, see the error codes above.\n"
    lines = ["| Code | Message |", "|------|---------|"]
    for error in errors:
        lines.append(f"| {error.get('code', '?')} | {error.get('message', '')} |")
    return "\n".join(lines) + "\n"


def render_example(example):
    """Render an example as request/response JSON fences."""
    lines = []
    if "name" in example:
        lines.append(f"**{example['name']}:**")
        lines.append("")
    params = example.get("params", {})
    request = {"jsonrpc": "2.0", "id": 1, "method": "<method>", "params": params}
    lines.append("~~~{.json}")
    lines.append(json.dumps(request, indent=2).replace('"<method>"', '"<method>"'))
    lines.append("~~~")
    if "result" in example:
        response = {"jsonrpc": "2.0", "id": 1, "result": example["result"]}
        lines.append("")
        lines.append("~~~{.json}")
        lines.append(json.dumps(response, indent=2))
        lines.append("~~~")
    return "\n".join(lines)


def generate():
    """Generate the content of the documentation page."""
    api = json.loads(API_FILE.read_text(encoding="utf-8"))
    info = api.get("info", {})
    lines = []
    lines.append("RPC Server API {#doc_rpc_api}")
    lines.append("===============\n")
    lines.append("[TOC]\n")
    lines.append(
        "The RPC server allows external tools (e.g. MCP servers, plugins or "
        "scripting libraries) to control a running LibrePCB application. "
        "This page documents the transport protocol and all available "
        "commands. The commands are generated from "
        "`share/librepcb/rpc/api.json`, so the same information is also "
        "available at runtime through the `rpc.discover` method.\n"
    )
    lines.append(
        "> Note: This page is about controlling a *running* LibrePCB "
        "application. For the *online* REST API used to fetch libraries, see "
        "@ref doc_server_api.\n"
    )
    lines.append(PROTOCOL_INTRO)
    lines.append("\n")
    lines.append("# Commands {#doc_rpc_api_commands}\n")
    lines.append(
        "All commands below are dispatched to the application and executed "
        "on its GUI thread. If not stated otherwise, commands are read-only "
        "and do not modify any state. Mutating commands operate on the "
        "undo stack, i.e. the user can undo them in the GUI.\n"
    )
    for method in api.get("methods", []):
        name = method.get("name", "?")
        lines.append(f"## {name} {{#{anchor_for(name)}}}\n")
        if "summary" in method:
            lines.append(f"{method['summary']}\n")
        if "description" in method:
            lines.append(f"{method['description']}\n")
        lines.append("**Params:**\n")
        lines.append(render_params_table(method))
        lines.append("**Result:**\n")
        result_section = render_result_section(method)
        if result_section:
            lines.append(result_section)
        else:
            lines.append("None.\n")
        lines.append("**Errors:**\n")
        lines.append(render_errors_table(method))
        examples = method.get("examples", [])
        if examples:
            lines.append("**Examples:**\n")
            for example in examples:
                rendered = render_example(example)
                rendered = rendered.replace('"<method>"', f'"{name}"')
                lines.append(rendered)
                lines.append("")
        lines.append("---\n")
    # Remove the trailing separator after the last method.
    if lines and lines[-1] == "---\n":
        lines.pop()
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--check",
        action="store_true",
        help="Only check if the committed documentation is up to date",
    )
    args = parser.parse_args()

    content = generate()
    if args.check:
        try:
            current = DOCS_FILE.read_text(encoding="utf-8")
        except FileNotFoundError:
            print(f"ERROR: {DOCS_FILE} does not exist, run the generator!")
            return 1
        if current == content:
            print("RPC API documentation is up to date.")
            return 0
        print(
            f"ERROR: {DOCS_FILE} is not up to date with {API_FILE}!\n"
            "       Run 'python3 dev/generate_rpc_api_docs.py' and commit "
            "the result."
        )
        return 1

    DOCS_FILE.write_text(content, encoding="utf-8")
    print(f"Generated {DOCS_FILE}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
