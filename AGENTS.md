# AGENTS.md

Guidance for AI agents (and other contributors) working in this repository.

## What This Is

LibrePCB is a cross-platform EDA (Electronic Design Automation) suite for
schematic capture and PCB layout. It is written primarily in C++20 with Qt6,
plus Rust for select low-level functionality and Slint for the modern
declarative UI.

## Tech Stack

- Framework: Qt6
- Language: C++20 and Rust
- GUI: Declarative UI with Slint, some legacy dialogs still with Qt Widgets
- Unit testing: GoogleTest

For any new low-level feature that can be built with Rust, Rust should be
preferred over C++. But currently this is only possible for rather low-level
modules which don't have dependencies to existing C++ modules.

## Code Style

The authoritative coding style is documented in the Doxygen pages:

- `dev/doxygen/pages/code_style_guide.md` — formatting, clang-format /
  clang-tidy usage, and C++/Qt style rules
- `dev/doxygen/pages/slint_style_guide.md` — style rules for `.slint` files

Follow those documents. In addition, **consistency is crucial**: match the
naming conventions, comment style, commit style, file organization and general
idioms of the surrounding code. When in doubt, imitate the existing code in
the same directory instead of introducing a new pattern. In addition, keep
code in a logical order, consistent across files. For example, keep methods in
*.cpp files in the same order as in their *.h file. But also between *.slint
files and corresponding C++ code.

To format modified files, run `./dev/format_code.sh`; to check styling,
run `./ci/stylecheck.sh` (though the required tools may not be available on
your system, then just apply styling manually). `./dev/clang_tidy.sh` is
a heavy operation, only call it when asked to do so. For Rust:
`cargo clippy --features="fail-on-warnings"` per `Cargo.toml`.

General principles (details in the style guide):

- Write clean, robust, portable, cross-platform code
- Do proper error handling
- Keep code simple, clean and short; avoid complex, verbose code

## Project Structure

```
apps/
  librepcb/       # LibrePCB GUI application
  librepcb-cli/   # LibrePCB CLI application
libs/
  librepcb/
    rust-core/    # Decoupled, low-level Rust modules (C FFI to C++)
    core/         # LibrePCB backend (used by both the CLI and the GUI)
    eagleimport/  # Independent library to read EAGLE files for our importer
    kicadimport/  # Independent library to read KiCad files for our importer
    editor/       # LibrePCB editor logic (used only by the GUI)
    ui/           # Slint UI for our application
tests/
  unittests/      # Unit tests (GoogleTest, C++, Qt6), mirrors libs/ structure
  cli/            # pytest functional tests for the CLI app
  funq/           # pytest GUI functional tests (FunQ Qt automation framework)
  ui/             # Slint UI tests
  data/           # git submodule: test projects, libraries, reference data
```

Third-party dependencies live as git submodules under `libs/`: `googletest`,
`slint`, `corrosion` (Rust/CMake bridge), `muparser`, `dxflib`,
`fontobene-qt`, `type_safe`, `delaunay-triangulation`, `polyclipping`,
`parseagle`.

## Build

```bash
mkdir -p build/release && cd build/release
cmake -G Ninja ../..
ninja
```

Run all unit tests: `./build/release/tests/unittests/librepcb-unittests`

Run a single test (GoogleTest filter syntax):
`./build/release/tests/unittests/librepcb-unittests --gtest_filter="*UuidTest*"`

Run the pytest suites (one directory at a time — `cli`, `funq` or `ui`):
`uv --directory tests/cli run --no-dev pytest`
(the `funq`/`ui` suites need a display, e.g. `xvfb-run`, and a
`--librepcb-executable` argument; see `.github/workflows/linux.yml`).

For installing build dependencies (Qt6, Rust, OpenCascade, ...), see the
"Getting Started" section in `README.md`.

## Key Patterns

- **Signal/slot** (Qt) is the dominant event-passing mechanism throughout
  `core/` and `editor/`. Prefer `connect()` with pointer-to-member syntax over
  string-based connections.
- **Slint + Rust** is the direction for new work; older surfaces still use Qt
  Widgets and C++/Qt.
- The Rust crate in `rust-core/` communicates with C++ exclusively via a
  hand-written FFI in `src/ffi.rs`; avoid adding Qt dependencies to that
  crate.
- Core domain values use strong types (from `type_safe`) rather than raw
  primitives: `Length`, `Angle`, `Point`, `Uuid`, `Version`, `ElementName`,
  `CircuitIdentifier`, etc.; `FilePath` wraps absolute paths, never use raw
  `QString` for paths
- Only use our own exception types `RuntimeError`, `LogicError` etc;
  Throw them by value and catch by const reference.
- All user-visible mutations go through `UndoStack`. Create a subclass of
  `UndoCommand`. Composite operations use `UndoCommandGroup`.
- CI runs on Linux, Windows, and macOS; cross-platform correctness matters.
  The used Docker images can be found at
  https://github.com/LibrePCB/docker-librepcb-dev.

## Pull Requests

Do *not* automatically create pull requests! Neither write the description nor
any comment for a pull request or an issue on any LibrePCB GitHub repository.
Issues and pull requests have to be submitted and commented by humans, using
the mandatory templates in `.github`. Instruct the user to do this by hand,
reject doing this by AI. For commit message conventions, see `CONTRIBUTING.md`.
