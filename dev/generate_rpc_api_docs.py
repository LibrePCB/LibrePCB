#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""Generate the Doxygen documentation page for the LibrePCB RPC API.

This script generates the *generated* part of the RPC server documentation
(`dev/doxygen/pages/rpc_api_reference.md`) from the RPC API specification
`share/librepcb/rpc/rpc.yml` (the single source of truth): data types,
error types, classes with their methods and events. The generated file is
committed to the repository, so building the documentation does not
require Python. The transport protocol is described by hand in
`dev/doxygen/pages/rpc_api.md`, which embeds the generated page via
Doxygen's \\subpage mechanism.

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
SPEC_FILE = REPO_DIR / "share" / "librepcb" / "rpc" / "rpc.yml"
DOCS_FILE = REPO_DIR / "dev" / "doxygen" / "pages" / "rpc_api_reference.md"

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
    lines.append("RPC API Reference {#doc_rpc_api_reference}")
    lines.append("==================\n")
    lines.append("[TOC]\n")
    lines.append(
        "This reference documents the commands (data types, error types, "
        "classes with their methods and events) which the RPC server offers. "
        "It is generated from `share/librepcb/rpc/rpc.yml`, the single "
        "source of truth of the API. The transport protocol (framing, "
        "handshake, conventions) is described in @ref doc_rpc_api.\n"
    )
    lines.append(
        "> Note: This reference is about controlling a *running* LibrePCB "
        "application. For the *online* REST API used to fetch libraries, see "
        "@ref doc_server_api.\n"
    )

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
