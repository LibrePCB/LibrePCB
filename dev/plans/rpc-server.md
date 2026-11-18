# Plan: TCP RPC Server for LibrePCB

Base for three upcoming features: MCP server (`librepcb-mcp`), plugin
binaries, and scripting libraries. All three are just *clients* of one
in-process server owned by the GUI application.

Build directory: `build/agent` (per CLAUDE.md). No commits unless asked.

## 1. Architecture

```
┌─────────────────────────── apps/librepcb (GUI process) ───────────────────────────┐
│                                                                                     │
│  GuiApplication ──owns──> RpcServer (editor/, QObject, main thread)                │
│                              │  ▲ commands (QJsonObject)      │                     │
│                              │  └─ BlockingQueuedConnection │ events (signals →   │
│                              ▼                                │  ffi_rpc_server_notify)│
│                        rust-core::rpc_server (std::net, own threads)                │
│                        JSON-RPC 2.0 · auth · NDJSON · rpc.discover                  │
└──────────────────────────────────┬──────────────────────────────────────────────────┘
                                   │ TCP 127.0.0.1:<ephemeral> + token auth
        ┌──────────────────────────┼──────────────────────────┐
   librepcb-mcp (stdio MCP)   plugin binaries          Python scripts
   — near pass-through         (any language)          (stdlib-only client)
```

### Confirmed decisions

| Topic | Decision |
|---|---|
| Transport | TCP bound to `127.0.0.1`, OS-assigned ephemeral port; opaque token for auth (**hardcoded in v1** — instance registry deferred, see section 9) |
| Wire protocol | JSON-RPC 2.0, newline-delimited (NDJSON) |
| GUI-thread dispatch | Synchronous: connection thread blocks while command executes on the main thread via `BlockingQueuedConnection` (precedent: `libs/librepcb/core/export/graphicsexport.cpp:59`) |
| Rust concurrency | std threads only, reader/writer thread pair per connection (no tokio, no mio) |
| Events | Basic JSON-RPC notifications in v1 (`event/projectOpened`, `event/projectClosed`, `event/selectionChanged`) |
| Scope | GUI-attached only; no headless/CLI support; no Python driver in v1 |
| API definition | OpenRPC document (`api.json`) as single source of truth |
| Docs | Generated Doxygen-flavored Markdown page, committed, rendered into existing Doxygen site |
| Multi-instance | Per-user instance registry in the *runtime* directory (`$XDG_RUNTIME_DIR` on Linux — shared identically with Flatpak/Snap sandboxes); **deferred for v1** (hardcoded token, see section 9) |

Why not gRPC/WebSocket/binary formats: rejected earlier — control-plane traffic
is tiny and human/agent-paced; JSON-RPC 2.0 is the same envelope MCP uses, so
the MCP glue becomes near pass-through and any future client (Python stdlib,
Node, C++) is trivial.

## 2. Protocol specification

### Framing

One JSON-RPC 2.0 message per line (NDJSON). `serde_json` never emits raw
newlines, so line framing is safe. Debuggable with `nc 127.0.0.1 <port>`.

### Handshake

- First message from the client **must** be `initialize` carrying the token:
  ```json
  {"jsonrpc":"2.0","id":1,"method":"initialize","params":{"token":"<opaque token>"}}
  ```
- Wrong/missing token → server responds with error `-32001` and closes the
  connection immediately.
- Success result:
  ```json
  {"jsonrpc":"2.0","id":1,"result":{"protocolVersion":"1","appVersion":"<App version>","pid":1234,"workspace":"<path>"}}
  ```

  The info is a startup snapshot provided by C++; live data (e.g. open
  projects) is available through commands like `project/list` after
  connecting.

### Protocol-level methods (handled in Rust, no C++ involved)

- `initialize` — handshake/auth (see above)
- `ping` — returns `{}`
- `rpc.discover` — returns the OpenRPC schema document (OpenRPC standard
  method) passed in via FFI at startup

Unknown-to-Rust methods are forwarded to the C++ dispatch callback.

### Error codes

JSON-RPC standard: `-32700` parse error, `-32600` invalid request,
`-32601` method not found, `-32602` invalid params, `-32603` internal error.
Application range (documented in `api.json`): `-32001` unauthenticated,
`-32002` command failed, `-32003` project not open, `-32004` server shutting
down. C++ handlers control their error code/message.

### Notifications (server → client, no `id`)

```
{"jsonrpc":"2.0","method":"event/projectOpened","params":{"project":"<uuid>"}}
```

### Request semantics (v1)

Each connection processes its requests strictly sequentially (reader thread
blocks in dispatch). Document this; clients wanting parallelism open multiple
connections. Serialization through the GUI thread means **no locking** — the
main thread is the single writer for all editor state.

### Method namespaces

`project/*`, `schematic/*`, `board/*`, `undostack/*`, `editor/*`, plus
protocol-level `initialize`/`ping`/`rpc.discover`. JSON keys in camelCase.
Params/results are JSON; C++ converts to strong types (`Uuid`, `Length`, ...).

## 3. rust-core implementation (Qt-free)

### Module layout

```
libs/librepcb/rust-core/src/rpc_server.rs       (pub mod rpc_server; in lib.rs)
libs/librepcb/rust-core/src/rpc_server/
  protocol.rs        — JSON-RPC 2.0 envelope: message classification
                       (request/notification/response/invalid), RpcError,
                       error codes, response/notification serialization
  codec.rs           — read_message(&mut impl BufRead) / write_message(&mut impl Write)
                       NDJSON framing; transport-agnostic (later UDS support)
  connection.rs      — accept loop, per-connection reader/writer threads,
                       handshake enforcement, message processing, dispatch
libs/librepcb/rust-core/src/ffi/rpc_server_ffi.rs   (register in src/ffi.rs, cbindgen → ffi.h)
```

Note: `RpcServer`/`RpcServerConfig`/`DispatchFn` live in the module root
`rpc_server.rs` (a `server.rs` inside `server/` would violate clippy's
`module_inception`, which this repo denies). Implemented in milestone 1.

### RpcServer internals

```rust
pub struct RpcServer { inner: Arc<Inner> }  // opaque handle via cbindgen

struct Inner {
    token: String,                       // opaque string from C++ (hardcoded in v1;
                                         // later part of the ProcessFile instance
                                         // data, see section 9)
    dispatch: (RpcDispatchFn, ctx),       // forwarded to C++
    api_schema: Mutex<Option<String>>,   // served by rpc.discover
    clients: Mutex<HashMap<ConnId, mpsc::Sender<String>>>,  // writer threads
    running: AtomicBool,
}
```

- `start()`: bind `127.0.0.1:0`, report the actual port to C++; spawn accept
  loop thread.
- Per connection: **reader thread** (`TcpStream::try_clone` + `BufReader`,
  blocks on `read_line`): enforce handshake, then for each message: parse
  envelope → handle `ping`/`rpc.discover` inline → else call the dispatch
  callback (blocks until C++ answered) → write response via the connection's
  channel. **Writer thread**: owns the write half, blocks on its `mpsc`
  channel; receives responses *and* broadcast notifications. `notify()` is
  non-blocking (`try_send`, ignore failure; writer thread cleans up its
  registry entry when the peer is gone).
- Use **unbounded** `std::sync::mpsc` channels so `ffi_rpc_server_notify` called
  from the GUI thread can never block.
- Protocol errors (malformed JSON, wrong envelope) → standard JSON-RPC error
  responses, never drop the connection.

### New Cargo dependencies

`serde_json` only — already in `Cargo.lock` transitively; it becomes a
direct dep (no serde derive needed, the envelopes are built manually with
`serde_json` values). No async runtime, no socket crates. Implemented in
milestone 1.

### FFI surface (cbindgen, ibom handle pattern)

```rust
pub type RpcDispatchFn = extern "C" fn(
    ctx: *mut c_void,
    method: &QString,      // e.g. "project/list"
    params: &QString,      // raw JSON object (params), parse in C++
    response: &mut QString // ok=true: raw JSON result value
                           // ok=false: JSON error object {"code":..,"message":..}
) -> bool;

ffi_rpc_server_new(token: &QString, protocol_version: &QString, info: &QString,
                   dispatch: RpcDispatchFn, ctx: *mut c_void) -> *mut RpcServer;
ffi_rpc_server_set_api_schema(s: &mut RpcServer, schema: &QString);
ffi_rpc_server_start(s: &mut RpcServer, port: u16, out_port: &mut u16) -> bool;
ffi_rpc_server_notify(s: &mut RpcServer, method: &QString, params: &QString);
ffi_rpc_server_delete(s: *mut RpcServer);
```

Notes:
- `protocol_version` and `info` (a raw JSON object, merged into the
  `initialize` result) are passed at creation time.
- `port` in `ffi_rpc_server_start()` is the desired port (0 = OS-assigned);
  the actual port is reported through `out_port`.
- Rust calls back into C++ for `QString` allocation
  (`ffi_qstring_new`/`ffi_qstring_delete` in `ffi.cpp`).
- The dispatch callback runs on the Rust connection threads. Since raw
  function pointers are not `Send`/`Sync` when their signature contains raw
  pointers, the Rust FFI wraps callback + context in a `CppDispatch`
  newtype, and calls it through a helper function to force the closure to
  capture the struct as a whole (Rust's disjoint closure captures would
  otherwise capture the raw fields individually).
- Implemented in milestone 2; cbindgen generates the `RpcDispatchFn`
  function-pointer typedef and an opaque `RpcServer` handle.

Note: the `token` parameter stays in the FFI surface even after the
ProcessFile registry is implemented (C++ reads the token from the instance
data and passes it here) — no FFI change needed for section 9.

Reverse-FFI (Rust calling C++) follows the existing `cpp_ffi.rs` pattern;
string passing via `&QString` like existing modules.

## 4. C++ implementation

### Files

```
libs/librepcb/editor/rpc/rpcserver.{h,cpp}
libs/librepcb/editor/rpc/commands.cpp           (split further when it grows)
```

New files get the standard GPL header; follow the repo style guide
(`./dev/format_code.sh`; run `./ci/stylecheck.sh` if tools available).

### `RpcServer` (QObject, created by `GuiApplication`)

Note: The C++ class `RpcServer` (namespace `librepcb::editor`) wraps the
cbindgen-generated FFI handle `rs::RpcServer` (namespace `librepcb::rs`).
Always reference the latter with the explicit `rs::` prefix to keep the two
apart.

Responsibilities:

1. **Lifecycle**: created only if the new workspace setting is enabled;
   stopped early in teardown (see shutdown ordering below).
2. **Token + instance registry** — *final design deferred, see section 9*:
   - Interim (v1): a **hardcoded token** (development only — see risks).
   - Final design: a Rust `ProcessFile` module writes
     `<QStandardPaths::RuntimeLocation>/librepcb/instances/<uuid>.json`
     (plus a `.lock` file) at server start and removes both at teardown.
     C++ only fills the instance data (a Rust struct passed via FFI,
     including the token) and owns the handle.
   - **Why `RuntimeLocation` (i.e. `$XDG_RUNTIME_DIR` on Linux) and not
     `AppLocalDataLocation`:** Flatpak and Snap bind-mount the runtime
     directory *identically* into the sandbox, so the file is found at the
     same path from inside and outside the sandbox (clients may run on
     either side). It is also per-user tmpfs with mode `0700`, and its
     per-session lifetime means stale entries never accumulate across
     reboots. On Windows (no sandbox) the Qt fallback is a per-user temp
     location, which is fine.
3. **API schema**: read `share/librepcb/rpc/api.json` via
   `Application::getResourcesDir()` (handles dev builds, installed
   `share/` and the `LIBREPCB_SHARE_DIR` override), pass content via
   `ffi_rpc_server_set_api_schema`.
4. **Dispatch callback** (runs on Rust connection threads!): marshal to the
   main thread with
   `QMetaObject::invokeMethod(this, [...]{...}, Qt::BlockingQueuedConnection)`
   → look up `method` in the command registry → execute handler → serialize
   result/error into `response`. Catch all `librepcb` exceptions
   (`catch (const Exception& e)`) and convert to `-32002` responses —
   exceptions must never cross the FFI boundary.
5. **Command registry**: `QHash<QString, Handler>` with
   `using Handler = std::function<Result(const QJsonObject&)>` where
   `Result` carries either a `QJsonValue` result or an error code+message.
   Registry keys must match `api.json` method names exactly (unit-tested).
6. **Events**: connect existing `GuiApplication`/`ProjectEditor` signals →
   `ffi_rpc_server_notify(...)`. v1: `projectOpened`, `projectClosed`,
   `selectionChanged` (add a signal where missing; keep minimal).

### v1 command set (keep it small)

- `project/list` — open projects + metadata
- `project/get` — project overview (sheets, metadata)
- `undostack/undo`, `undostack/redo`, `undostack/getStatus`
- `editor/executeCommand` — pass-through to `EditorCommand`s from
  `editorcommandset.h` (cheap, broad agent coverage; check applicability
  against the active window/tab per command)

All mutating handlers push proper `UndoCommand`s onto the project's
`UndoStack` (mandatory per AGENTS.md). This is a feature: remote/agent edits
appear in the user's undo history; the GUI stays the single source of truth.

### Workspace setting

`WorkspaceSettings`: setting `enable_rpc_server` (default off), following
the existing `WorkspaceSettingsItem_GenericValue<bool>` pattern. **Status:**
load/save implemented; the checkbox in the workspace settings dialog is
still missing (to be added together with the random token, when the
feature becomes user-facing).

For testing, the server can also be forced with the environment variable
`LIBREPCB_ENABLE_RPC_SERVER=1` (following the pattern of
`LIBREPCB_ENABLE_TEST_ADAPTER`), and `LIBREPCB_RPC_PORT` forces a specific
port (otherwise the OS assigns one).

### Shutdown ordering (deadlock safety — the one real hazard)

Rule to document in `rpcserver.h`: **code running on the GUI thread must
never synchronously call back into the server** — `ffi_rpc_server_notify` is fine
(non-blocking channel push); `ffi_rpc_server_stop`/issuing requests would deadlock.

Teardown sequence in the C++ `RpcServer` (called from `GuiApplication` while the
event loop still runs):

1. Set a "shutting down" atomic so new dispatches answer `-32004` instead of
   marshalling to the GUI thread.
2. Close listener + client sockets (unblocks reader/writer threads; an
   in-flight dispatch drains because the event loop still runs).
3. Join connection threads (bounded timeout; on timeout log a warning and
   detach — safe during process teardown).
4. `ffi_rpc_server_delete`.

## 5. API schema & documentation

### Single source of truth

`share/librepcb/rpc/api.json` — OpenRPC 1.2.x document:

- Located in the `share/` directory on purpose: the existing
  `Application::getResourcesDir()` mechanism resolves it both for
  developer builds and installed packages (the `share/*` glob install
  rule picks it up automatically), and `LIBREPCB_SHARE_DIR` can override
  the location for tests.

- `info`: title, `protocolVersion` (matches `initialize` result), app version
- `methods[]`: name, summary/description, params (JSON Schema + description),
  result schema, errors (app codes), `examples[]` (used as test fixtures)
- Curated subset marked for MCP later via `x-mcp` extension on methods
- Split with `$ref` per domain (`api/project.json`, ...) once it grows

### Docs pipeline (no Node, no new toolchain)

```
api.json ──> dev/generate_rpc_api_docs.py (stdlib-only, ~150 lines)
         └──> dev/doxygen/pages/rpc_api.md   (generated, committed)
                  └──> rendered by existing Doxygen site (dev/doxygen/make.sh)
```

- Generator emits Doxygen-flavored Markdown following repo conventions:
  `{#doc_rpc_api}` page anchor, `[TOC]`, `~~~{.json}` fences, param
  tables (flatten simple schemas; link full schema for complex ones).
- The generated `.md` is **committed** (like autogenerated `ffi.h`), so the
  docs build needs no Python.
- Cross-link with the existing `server_api.md` (that page documents the
  *online REST API* — different topic, clarify at top of both pages).
- **CI stale-check**: workflow step regenerates and diffs; fail if
  `api.json` was edited without re-running the generator.

### Drift protection

1. Unit test: parse `api.json` with `QJsonDocument`, compare method name set
   against the C++ command registry (both directions).
2. funq tests use the schema's `examples` as request payloads where sensible.

## 6. Testing

- **rust-core** (`cargo test`, without `ffi` feature per README):
  envelope (de)serialization incl. untagged ids, NDJSON codec edge cases,
  handshake/auth flow (wrong token → close), dispatch via mock closure,
  notify broadcast, sequential-per-connection semantics.
- **C++ unittests** (GoogleTest, mirror `libs/` structure): registry vs.
  `api.json` consistency; dispatch error mapping (exception → `-32002`).
- **funq/pytest e2e**: enable the setting in a test workspace, start app,
  connect a socket client from the test, `initialize`, call commands, assert
  results and undo-stack effects; `nc` for manual smoke testing.
- Cross-platform: no platform-specific code expected on the Rust side
  (std::net only); `QStandardPaths` handles the registry dir per-platform.

## 7. Milestones

1. **rust-core `rpc_server` module + cargo tests** — fully standalone, no C++.
   **Status: Done** (implemented, 166 tests pass, clippy clean).
2. **FFI + C++ skeleton** — `ffi/rpc_server_ffi.rs`, cbindgen integration,
   C++ `RpcServer`, workspace setting, hardcoded token (instance registry
   deferred, see section 9), `api.json` with first commands (`project/list`,
   `project/get`, `undostack/*`), docs generator + generated `rpc_api.md` +
   CI stale-check, registry/schema unit test, funq smoke test.
   **Status: Done.** Implemented and verified: 166 Rust tests pass, clippy
   clean, 2598 C++ unittests pass (incl. the new registry-vs-schema test),
   and an end-to-end smoke test against the real (offscreen) application
   passed all protocol and command checks (handshake, ping, project/list,
   error codes, schema discovery, wrong-token disconnect, clean shutdown).
3. **Events + `editor/executeCommand`** — notification wiring,
   pass-through command, documented in `api.json`.
4. **`apps/librepcb-mcp`** (separate PR) — stdio MCP server: instance
   discovery via registry, token, allowlist-driven tool surface from
   `api.json` (MCP `tools/list` is JSON-Schema-based → near pass-through).
5. **Later (additive)**: Python driver (~200 lines, stdlib-only, dynamic
   methods), quicktype-generated type models, UDS/named-pipe transport
   option, async dispatch, client event subscriptions, runtime JSON Schema
   validation in Rust (`jsonschema` crate) — only if real need arises.

## 8. Risks / gotchas

- **Blocking dispatch deadlocks**: only possible if GUI-thread code calls
  back into the server synchronously — forbidden by documented rule; teardown
  sequence above handles the in-flight case.
- **Modal dialogs / long GUI operations** delay commands but don't break
  them (modal dialogs spin an event loop). Document as v1 semantics; clients
  handle slow responses.
- **Never let exceptions cross the FFI**; always map to error responses.
- **Unbounded channels** so GUI-thread `notify` can't block.
- Bind loopback only; close unauthenticated connections immediately; token
  file with `0600`-equivalent permissions; log connections (existing debug
  logging).
- **Hardcoded token is development-only**: while the instance registry (and
  thus a random per-instance token) is deferred, *any* local process can
  connect to the RPC server. Acceptable during development; must be
  replaced before shipping (the ProcessFile design in section 9 fixes this).
- **Sandboxing (Flatpak/Snap)**: TCP on 127.0.0.1 works because sandboxes
  share the host network namespace — *if* network permissions are granted.
  Flatpak needs `share=network` (LibrePCB has it for library downloads);
  Snap needs the `network` **and** `network-bind` plugs (packaging
  requirement to document). The instance file must live in
  `QStandardPaths::RuntimeLocation` (see sections 4 and 9) — it is the only
  standard location which is identical inside and outside a sandbox. The
  recorded PID is sandbox-internal and informational only; liveness is
  determined by connect + handshake (later: cheap pre-filtering via the
  ProcessFile lock file, see section 9). Manually test discovery once with a
  Flatpak build (server inside sandbox, client on host, and vice versa).
- cbindgen must handle the `RpcDispatchFn` typedef and opaque
  `rs::RpcServer` handle (same style as `InteractiveHtmlBom`).
- Keep rust-core Qt-free (AGENTS.md): the `rpc_server` module uses only
  std + `serde_json`.

## 9. Deferred design: `ProcessFile` (instance registry)

The instance registry was deferred to keep the RPC server feature simple
and straightforward. Interim state: **hardcoded token** (development only,
see risks). The following is the agreed design for the follow-up.

### Concept

- A generic Rust module `ProcessFile` (or similar) in rust-core, not tied to
  `rpc_server`:
  - `new(...)`: Creates `<$XDG_RUNTIME_DIR>/librepcb/instances/<uuid>.json`
    with the instance data, plus an empty lock file at the same path with an
    additional `.lock` suffix, locked with `std::fs::File::lock()` for the
    holder's lifetime. The JSON content comes from a Rust struct which is
    filled from C++ and passed to `new()`.
  - `update(data)`: Atomically replaces the JSON file (write to a temporary
    file + rename) with the new content.
  - `Drop`: Deletes both files. C++ calls `new()` and stores the handle in a
    smart pointer / RAII wrapper which automatically calls drop on free.
- Where the filename UUID comes from does not matter — possibly passed as a
  string to `new()`, coming from C++ for now (e.g. `QUuid::createUuid()`),
  maybe replaced by the `uuid` crate later.
- The RPC token is part of the ProcessFile data structure, filled by C++
  for now. The RPC server receives the token when it is started (or so) —
  it does not need to know that the token comes from a ProcessFile. The
  token is considered an *opaque string*; whether it is a UUID or something
  else is an implementation detail.
- Clients can (optionally) use the lock file for stale file detection
  (process crashed → the OS released the lock → a client's `try_lock()`
  succeeds). A heartbeat (updating the JSON file with a timestamp at
  regular intervals) is an alternative — to be decided later.
- Base directory: `$XDG_RUNTIME_DIR` on Linux (see section 4 for why this is
  the only sandbox-proof location). *Open point:* whether `ProcessFile`
  resolves the base directory itself (reads `XDG_RUNTIME_DIR` / temp dir) or
  receives it from C++ (`QStandardPaths::RuntimeLocation`, matching the
  FilePath convention and providing the Windows fallback) — to be decided
  when implementing.

### Review notes

- **Never lock the JSON itself** — `update()` replaces it via rename, which
  would silently invalidate a lock held on the old inode (readers of the new
  file would see no lock). The separate, never-rewritten `.lock` file keeps
  the lock stable across updates.
- **Keep the locked `File` handle alive in the struct** — the lock is
  released when the descriptor closes; dropping it early would make a live
  instance look stale to clients.
- `std::fs::File::lock()` / `try_lock()` were stabilized in Rust 1.89 —
  verify the CI toolchain images provide it (local toolchain: rustc 1.99.1).
- On Windows, `std::fs::rename` over an existing file behaves differently
  than POSIX rename; verify the atomic-replace behavior in tests (a small
  non-atomic window is acceptable for a discovery file).
- Lock-based stale detection makes a heartbeat redundant for crash detection
  (a heartbeat would only distinguish "hung but alive"). Skip the heartbeat
  in v1; `update()` remains for content refreshes (the bound port after
  server start, project list, ...).
- Ordering: `ProcessFile::new()` before starting the RPC server (the token
  is read from the instance data to configure it), then `update()` with the
  bound port.
- Discovery flow once implemented: scan the directory → skip entries whose
  `.lock` file can be locked (stale) → connect + `initialize` handshake for
  the rest. The recorded PID is informational only (Flatpak sandboxes run in
  their own PID namespace).
- If `new()` fails (e.g. no writable runtime directory), disable the RPC
  server feature with a log message instead of running undiscoverable.
