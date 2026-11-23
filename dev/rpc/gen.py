#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""Generate code and documentation for the LibrePCB RPC API.

This script generates two files from the RPC API specification
`dev/rpc/rpc.yml` (the single source of truth), using the Jinja2
templates from `dev/rpc/templates/`:

- `dev/doxygen/pages/rpc_api.md` — the Doxygen API reference.
  The transport protocol is described by hand in
  `dev/doxygen/pages/rpc_server.md`, which embeds the generated page via
  Doxygen's \\subpage mechanism.
- `libs/librepcb/rust-core/src/rpc_server/rpc_api.rs` — generated Rust
  code. For now it contains only the error codes with their canonical
  messages; more parts of the API may follow later.
- `libs/librepcb/editor/rpc/api/{classname}rpc.h` — the C++ command
  declarations, one static-only class per spec class (the method
  implementations are hand-written in the corresponding .cpp files).
- `libs/librepcb/editor/rpc/api/rpcerrors.h` — the error codes and one
  exception class per error code (the C++ error handling).
- `libs/librepcb/editor/rpc/api/rpctypes.h` — the enum types with helpers
  to read them from request arguments (`get`/`tryGet`).
- `libs/librepcb/editor/rpc/api/commands.cpp` — the generated command
  dispatcher (see the RPC server documentation).

Both files are committed to the repository, so building does not require
Python. Later, this script will also validate the specification file.

Usage:

    uv run gen.py        # From this directory
    uv run --directory dev/rpc gen.py

Attention: Do not modify the generated files manually, modify the
           specification file (or the templates) instead and re-run this
           script!
"""

import re
import sys
from pathlib import Path

import yaml
from jinja2 import Environment, FileSystemLoader

DIR = Path(__file__).resolve().parent
REPO_DIR = DIR.parent.parent
SPEC_FILE = DIR / "rpc.yml"
TEMPLATES_DIR = DIR / "templates"
DOCS_FILE = REPO_DIR / "dev" / "doxygen" / "pages" / "rpc_api.md"
RUST_FILE = REPO_DIR / "libs" / "librepcb" / "rust-core" / "src" / "rpc_server" / "rpc_api.rs"
EDITOR_RPC_DIR = REPO_DIR / "libs" / "librepcb" / "editor" / "rpc"
EDITOR_API_DIR = EDITOR_RPC_DIR / "api"
CMAKELISTS_FILE = REPO_DIR / "libs" / "librepcb" / "editor" / "CMakeLists.txt"
COMMANDS_FILE = EDITOR_API_DIR / "commands.cpp"
RPCERRORS_FILE = EDITOR_API_DIR / "rpcerrors.h"
RPCTYPES_FILE = EDITOR_API_DIR / "rpctypes.h"

CPP_STUB = """/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// TODO: Implement the RPC commands declared in the corresponding
// generated header.
"""


ENUM_DOC = (
    "Error codes for the RPC protocol, as listed in the error types"
    " section of the API specification. The codes are grouped: 10-19"
    " transport errors, 20-29 protocol errors, 30-39 command errors,"
    " 100- command execution errors. The C++ side has its own enum,"
    " generated from the same specification (see"
    " `editor/rpc/rpcerrors.h`)."
)


def cpp_class_name(cls):
    """The C++ class name for a yml class (null = the protocol-level
    methods, which are generated as a pseudo class "ProtocolRpc")."""
    return cls.get("name") or "Protocol"


def cpp_pascal(name):
    """Convert a lower_snake_case wire name into a PascalCase identifier."""
    return "".join(part.capitalize() for part in name.split("_"))


def rust_comment(text, prefix="///"):
    """Wrap a text into a Rust doc comment (ca. 80 columns)."""
    text = re.sub(r"\s+", " ", text).strip()
    lines = []
    line = prefix
    for word in text.split(" "):
        if len(line) + 1 + len(word) > 80:
            lines.append(line)
            line = prefix
        line += " " + word
    lines.append(line)
    return "\n".join(lines)


def cpp_brief(text):
    """Wrap a text into the lines of a C++ doxygen @brief block."""
    text = re.sub(r"\s+", " ", text).strip()
    lines = []
    line = ""
    for word in text.split(" "):
        if len(line) + 1 + len(word) > 66:
            lines.append(line)
            line = ""
        line += (" " if line else "") + word
    if line:
        lines.append(line)
    return "\n   *        ".join(lines)


def rust_string(s):
    """Escape a string into a Rust string literal."""
    return '"{}"'.format(s.replace("\\", "\\\\").replace('"', '\\"'))


def add_to_cmakelists(cmake, entry):
    """Add a missing source file entry to the rpc block of CMakeLists.txt.

    The entries are not sorted — the formatter takes care of that.
    """
    lines = cmake.splitlines(keepends=True)
    if any(line.strip() == entry for line in lines):
        return cmake
    rpc_indices = [
        i for i, line in enumerate(lines) if line.strip().startswith("rpc/")
    ]
    if not rpc_indices:
        raise RuntimeError("Could not find the rpc sources in CMakeLists.txt")
    lines.insert(rpc_indices[-1] + 1, f"  {entry}\n")
    return "".join(lines)


def make_environment():
    env = Environment(
        loader=FileSystemLoader(TEMPLATES_DIR),
        trim_blocks=True,
        lstrip_blocks=True,
    )
    env.filters["cpp_brief"] = cpp_brief
    env.filters["cpp_pascal"] = cpp_pascal
    env.filters["rust_comment"] = rust_comment
    env.filters["rust_string"] = rust_string
    return env


def strip_trailing_separators(content):
    """Remove trailing section separators (and blank lines)."""
    lines = content.splitlines()
    while lines and lines[-1] in ("---", ""):
        lines.pop()
    lines.append("")
    return "\n".join(lines) + "\n"


def main():
    spec = yaml.safe_load(SPEC_FILE.read_text(encoding="utf-8"))
    env = make_environment()

    DOCS_FILE.write_text(strip_trailing_separators(
        env.get_template("rpc_api.md.j2").render(spec=spec),
    ), encoding="utf-8")
    print(f"Generated {DOCS_FILE}")

    RUST_FILE.write_text(env.get_template("rpc_api.rs.j2").render(
        spec=spec, enum_doc=ENUM_DOC,
    ), encoding="utf-8")
    print(f"Generated {RUST_FILE}")

    for cls in spec.get("classes", []):
        name = cpp_class_name(cls)
        path = EDITOR_API_DIR / f"{name.lower()}rpc.h"
        path.write_text(env.get_template("command_class.h.j2").render(
            class_name=name,
            class_description=re.sub(r"\s+", " ", cls.get("description", "")),
            rpc_class=cls,
        ), encoding="utf-8")
        print(f"Generated {path}")

    # Make sure the command implementation files exist and are registered
    # in CMakeLists.txt (the headers are generated, the implementations
    # are written by hand).
    cmake = CMAKELISTS_FILE.read_text(encoding="utf-8")
    cmake_changed = False
    for cls in spec.get("classes", []):
        name = cpp_class_name(cls)
        base = EDITOR_API_DIR / f"{name.lower()}rpc"
        if not base.with_suffix(".cpp").exists():
            base.with_suffix(".cpp").write_text(CPP_STUB, encoding="utf-8")
            print(f"Created {base.with_suffix('.cpp')}")
        for filename in (f"{name.lower()}rpc.h",
                         f"{name.lower()}rpc.cpp"):
            entry = f"rpc/api/{filename}"
            new_cmake = add_to_cmakelists(cmake, entry)
            if new_cmake != cmake:
                cmake = new_cmake
                cmake_changed = True
    if cmake_changed:
        CMAKELISTS_FILE.write_text(cmake, encoding="utf-8")
        print(f"Updated {CMAKELISTS_FILE}")

    RPCERRORS_FILE.write_text(env.get_template("rpcerrors.h.j2").render(
        spec=spec,
    ), encoding="utf-8")
    print(f"Generated {RPCERRORS_FILE}")

    RPCTYPES_FILE.write_text(env.get_template("rpctypes.h.j2").render(
        spec=spec,
    ), encoding="utf-8")
    print(f"Generated {RPCTYPES_FILE}")

    COMMANDS_FILE.write_text(env.get_template("commands.cpp.j2").render(
        spec=spec,
    ), encoding="utf-8")
    print(f"Generated {COMMANDS_FILE}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
