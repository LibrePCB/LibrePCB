#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""Generate the Doxygen documentation page for the LibrePCB RPC API.

This script generates `dev/doxygen/pages/rpc_api.md` from the RPC API
specification `share/librepcb/rpc/rpc.yaml` (the single source of truth).
The generated file is committed to the repository, so building the
documentation does not require Python.

Usage:

    python3 dev/generate_rpc_api_docs.py    # Generate the docs

Attention: Do not modify the generated Markdown file manually, modify the
           specification file instead and re-run this script!

Requires PyYAML (development tool only, not needed for building the
application or the documentation).
"""

import sys
from pathlib import Path

import yaml

REPO_DIR = Path(__file__).resolve().parent.parent
SPEC_FILE = REPO_DIR / "share" / "librepcb" / "rpc" / "rpc.yaml"
DOCS_FILE = REPO_DIR / "dev" / "doxygen" / "pages" / "rpc_api.md"

# Static introduction about the transport protocol (everything except the
# methods, which are generated from the specification).
PROTOCOL_INTRO = """\
# Protocol {#doc_rpc_api_protocol}

The RPC server listens on a local TCP socket (`127.0.0.1`, port assigned by
the operating system) and accepts an unlimited number of connections. The
wire format is [MessagePack](https://msgpack.org), decoded directly from
the stream (self-delimiting, like the official MessagePack-RPC protocol,
no length prefix). A decoder which blocks mid-value until more bytes arrive
never re-parses partial data. The size of a single message is limited to
32 MiB (enforced mid-parse). If the stream contains invalid MessagePack,
the framing is broken and the connection is closed without a response.

## Handshake {#doc_rpc_api_protocol_handshake}

The first message of a connection must be an `initialize` request carrying
the authentication token. Any other first message (or a wrong token) leads
to an error response and an immediate disconnect. Also a previously
authenticated connection is disconnected if a second `initialize` request
contains a wrong token.

The success result is a map which contains the protocol version and some
application information:

~~~
[1, 1, nil, {"appVersion": "2.1.2", "pid": 1234,\\
"protocolVersion": "1", "workspace": "~/LibrePCB-Workspace"}]
~~~

Clients use `appVersion` together with the per-method `added_in_version`
from the specification file to determine which methods the server
supports: Methods added after the revision of the client's specification
are unknown to the client (calling them results in error `-32601`).

## Protocol-Level Methods {#doc_rpc_api_protocol_methods}

These methods are handled by the server itself, i.e. they are not
dispatched to the application:

| Method       | Description                                    |
|--------------|------------------------------------------------|
| `initialize` | Handshake & authentication (see above).        |
| `ping`       | Returns an empty map, useful to test a client.  |

## Messages {#doc_rpc_api_protocol_messages}

All messages are MessagePack arrays whose first element is the message
type:

| Type | Message         | Direction | Shape                     |
|------|-----------------|-----------|---------------------------|
| 0    | Request         | C→S       | `[0, msgid, method, params]` |
| 1    | Response        | S→C       | `[1, msgid, error, result]`  |
| 2    | Notification    | S→C       | `[2, method, params]`        |
| 3    | Request message | S→C       | `[3, msgid, payload]`       |
| 4    | Cancel          | C→S       | `[4, msgid]`                |

- `msgid` is an unsigned 32-bit integer, unique per connection while
  pending. Responses may be sent in any order (pipelining): The server
  keeps reading requests while previous ones are being processed.
- Exactly one response is sent per request, enforced by the server. A
  second response (or a response for an unknown or closed connection) is
  silently dropped.
- A request message (type 3) is only delivered while the request is
  pending, i.e. before its response. Commands may use it to report
  progress or to stream partial data (defined per method, none yet).
- With `[4, msgid]` a client may cancel a pending request. Cancellation
  is cooperative: The command stops its work when it can and responds
  with a "cancelled" error; if the work already completed, the normal
  result is sent. Either way, exactly one final response arrives.
- Unknown message types are ignored for forward compatibility. A request
  which doesn't conform to the envelope (e.g. a non-string method) is
  answered with error `-32600` and the connection stays open. However,
  values outside the protocol's value domain (e.g. `ext` values) result
  in error `-32602`.

## Params & Conventions {#doc_rpc_api_protocol_params}

The `params` of a request are a map (or nil, which means an empty map):
The arguments are passed as named fields, plus the reserved key `"obj"`
which carries the receiver handle for non-static methods (an argument may
never be named `obj`). The handle type is documented per class.

Values are from the restricted MessagePack domain: null, bool, int (i64),
float (f64), string, binary, array and map (with string keys). Enums are
transferred as strings (see the specification file for the allowed
values). Numeric conventions: Lengths in nanometers, angles in
microdegrees.

> Note: This page is about controlling a *running* LibrePCB application.
> For the *online* REST API used to fetch libraries, see @ref
> doc_server_api.
"""


def render_type(type_spec):
    """Render a type reference (a string or a dict with a "type" key)."""
    if isinstance(type_spec, str):
        return type_spec
    if isinstance(type_spec, dict):
        return type_spec.get("type", "unknown")
    return "unknown"


def render_description(type_spec):
    """Render the description of a type reference."""
    if isinstance(type_spec, dict):
        return type_spec.get("description", "")
    return ""


def render_fields(fields, heading="Fields:"):
    """Render a list of field definitions as a Markdown table."""
    if not fields:
        return ""
    lines = [
        f"**{heading}**",
        "",
        "| Name | Type | Description |",
        "|------|------|-------------|",
    ]
    for field in fields:
        name = field.get("name", "?")
        typ = render_type(field.get("type", "unknown"))
        # Nested item types are not part of the type column, so add them.
        items = field.get("items")
        if items:
            typ = f"array of {render_type(items)}"
        description = field.get("description", "")
        lines.append(f"| `{name}` | {typ} | {description} |")
    lines.append("")
    return "\n".join(lines)


def render_params(params):
    """Render the params of a method."""
    if not params:
        return "None.\n"
    return render_fields(params, heading="Params:")


def render_result(result):
    """Render the result of a method."""
    if result is None:
        return "None.\n"
    typ = render_type(result)
    description = render_description(result)
    items = None
    if isinstance(result, dict):
        items = result.get("items")
    lines = []
    if description:
        lines.append(f"{description}")
        lines.append("")
    lines.append(f"**Result:** `{typ}`")
    lines.append("")
    if isinstance(result, dict):
        fields = result.get("fields")
        if fields:
            lines.append(render_fields(fields))
        if items:
            lines.append(f"Array items: `{render_type(items)}`")
            lines.append("")
    return "\n".join(lines)


def render_errors(errors, error_types):
    """Render the error types a method may return."""
    if not errors:
        return "None (besides protocol-level errors).\n"
    lines = ["| Error | Code | Message |", "|-------|------|---------|"]
    for name in errors:
        error = error_types.get(name, {})
        lines.append(
            f"| `{name}` | {error.get('code', '?')} | "
            f"{error.get('message', '')} |"
        )
    return "\n".join(lines) + "\n"


def generate():
    """Generate the content of the documentation page."""
    spec = yaml.safe_load(SPEC_FILE.read_text(encoding="utf-8"))
    structs = {s["name"]: s for s in spec.get("structs", [])}
    enums = {e["name"]: e for e in spec.get("enums", [])}
    error_types = {e["name"]: e for e in spec.get("error_types", [])}
    lines = []
    lines.append("RPC Server API {#doc_rpc_api}")
    lines.append("===============\n")
    lines.append("[TOC]\n")
    lines.append(
        "The RPC server allows external tools (e.g. MCP servers, plugins or "
        "scripting libraries) to control a running LibrePCB application. "
        "This page documents the transport protocol and all available "
        "commands. It is generated from `share/librepcb/rpc/rpc.yaml`, the "
        "single source of truth of the API.\n"
    )
    lines.append(PROTOCOL_INTRO)
    lines.append("\n")

    # Data types.
    lines.append("# Data Types {#doc_rpc_api_types}\n")
    for struct in spec.get("structs", []):
        lines.append(f"## {struct['name']} {{#doc_rpc_api_type_{struct['name']}}}\n")
        if struct.get("description"):
            lines.append(f"{struct['description']}\n")
        lines.append(render_fields(struct.get("fields")))
        lines.append("---\n")
    for enum in spec.get("enums", []):
        lines.append(f"## {enum['name']} (Enum) {{#doc_rpc_api_type_{enum['name']}}}\n")
        if enum.get("description"):
            lines.append(f"{enum['description']}\n")
        lines.append("| Value | Description |")
        lines.append("|-------|-------------|")
        for value in enum.get("values", []):
            lines.append(
                f"| `{value.get('name', '?')}` | {value.get('description', '')} |"
            )
        lines.append("")
        lines.append("---\n")

    # Error types.
    lines.append("# Error Types {#doc_rpc_api_errors}\n")
    lines.append(
        "The `error` element of a response is a map with the keys `code` "
        "and `message` (and possibly additional properties, listed "
        "below):\n"
    )
    lines.append("| Error | Code | Message |")
    lines.append("|-------|------|---------|")
    for error in spec.get("error_types", []):
        lines.append(
            f"| `{error['name']}` {{#doc_rpc_api_error_{error['name']}}} | "
            f"{error.get('code', '?')} | {error.get('message', '')} |"
        )
    lines.append("")
    for error in spec.get("error_types", []):
        if error.get("description"):
            lines.append(f"**`{error['name']}`:** {error['description']}\n")
        properties = error.get("properties")
        if properties:
            lines.append(render_fields(properties, heading="Properties:"))
    lines.append("---\n")

    # Classes & methods.
    lines.append("# Commands {#doc_rpc_api_commands}\n")
    lines.append(
        "All commands below are dispatched to the application and executed "
        "on its GUI thread. Method names are `ClassName.method_name`. "
        "Non-static methods receive their receiver handle in the reserved "
        "`obj` key of the params.\n"
    )
    for cls in spec.get("classes", []):
        lines.append(f"## Class {cls['name']} {{#doc_rpc_api_class_{cls['name']}}}\n")
        if cls.get("description"):
            lines.append(f"{cls['description']}\n")
        if not cls.get("static") and cls.get("handle"):
            lines.append(f"**Handle (obj):** {cls['handle']}\n")
        for method in cls.get("methods", []):
            name = method.get("name", "?")
            full_name = f"{cls['name']}.{name}"
            lines.append(
                f"### {full_name} {{#doc_rpc_api_method_{cls['name']}_{name}}}\n"
            )
            if method.get("description"):
                lines.append(f"{method['description']}\n")
            lines.append(render_params(method.get("params")))
            lines.append(render_result(method.get("result")))
            lines.append("**Errors:**\n")
            lines.append(render_errors(method.get("errors", []), error_types))
            if method.get("added_in_version"):
                lines.append(f"**Added in version:** {method['added_in_version']}\n")
            lines.append("---\n")

    # Events.
    if spec.get("events"):
        lines.append("# Events {#doc_rpc_api_events}\n")
        for event in spec.get("events", []):
            lines.append(f"## {event['name']} {{#doc_rpc_api_event_{event['name']}}}\n")
            if event.get("description"):
                lines.append(f"{event['description']}\n")
            lines.append(render_result(event.get("params")))
            lines.append("---\n")

    # Remove the trailing separators.
    while lines and lines[-1] in ("---\n", ""):
        lines.pop()
    lines.append("")
    return "\n".join(lines)


def main():
    content = generate()
    DOCS_FILE.write_text(content, encoding="utf-8")
    print(f"Generated {DOCS_FILE}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
