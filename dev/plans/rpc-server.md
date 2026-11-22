# Plan: TCP RPC Server for LibrePCB (Protocol v2)

Base for three upcoming features: MCP server (`librepcb-mcp`), plugin
binaries, and scripting libraries. All three are just *clients* of one
in-process server owned by the GUI application.

**Status:** The first design (JSON-RPC 2.0, "v1") was implemented through
milestones 1–2 (Rust server module + C++ skeleton + command handlers + docs
pipeline, all working and tested). It was never released. This plan now
describes **protocol v2** (MessagePack-based, asynchronous dispatch), which
*supersedes* the v1 wire protocol. The v1 implementation is the starting
point: its threading model, lifecycle handling, auth flow and test
infrastructure survive; the envelope, codec and dispatch mechanism get
rewritten.

Build directory: `build/agent` (per CLAUDE.md). No commits unless asked.

## 1. Architecture

```
┌─────────────────────── apps/librepcb (GUI process) ──────────────────────────┐
│                                                                              │
│  GuiApplication ──owns──> RpcServer (editor/, QObject, main thread)          │
│                              ▲ commands (QVariant args)        │              │
│                              │ Qt::QueuedConnection (async!)  │ events       │
│                              │                                │ (type-2      │
│                              │                                │  broadcasts, │
│                              │ replies / progress (types 1,3) │  non-block.) │
│                              │                                ▼              │
│                        rust-core::rpc_server (std::net, own threads)         │
│                        msgpack envelope · auth · stream codec · QVariant FFI │
└──────────────────────────────┬───────────────────────────────────────────────┘
                               │ TCP 127.0.0.1:<ephemeral> + token auth
        ┌──────────────────────┼──────────────────────┐
   librepcb-mcp (stdio MCP)  plugin binaries        scripts (later)
   — value translation only   (any language)        (msgpack + socket)
```

### Confirmed decisions

| Topic | Decision |
|---|---|
| Transport | TCP bound to `127.0.0.1`, fixed default port 19536 ("LP" = 0x4C50) with fallback to higher ports; WebSocket *maybe* later, as a first-class endpoint or just a proxy plugin (to be decided when web clients materialize) |
| Wire format | MessagePack (`rmp` + `rmpv` on the Rust side; no `rmp-serde`, the envelope is built manually as values) |
| Envelope | Custom, msgpack-rpc-inspired arrays, extended with first-class request-scoped messages (progress/streaming, cancel); see section 2 |
| Framing | Self-delimiting msgpack stream decoding (like the official msgpack-rpc spec), no length prefix |
| C++ boundary | Rust converts msgpack ↔ `QVariant` and passes values through the FFI (C++ never sees msgpack; ~18 trivial FFI helpers in `ffi.cpp`) |
| Dispatch | **Asynchronous**: requests are handed off to the GUI thread via `Qt::QueuedConnection`; the GUI thread replies (and may emit progress) whenever it is ready |
| Method naming | `ClassName.method_name` (classes are first-class entities in the spec; see section 2.3) |
| Params | String-keyed map with a reserved `"obj"` key carrying the receiver handle — an *opaque* value (see section 2.4) |
| Domain types | Plain integers (`Length` in nm, `Angle` in µdeg, …) and f64 — **forever**, no msgpack `ext` types |
| Enums | Strings (self-describing; clients don't need the spec to display them) |
| Spec / discovery | YAML spec file (`dev/rpc/rpc.yml`, dev tool, not shipped); **no `rpc.discover` method** — clients read the spec file and compare per-function `since` with the `version` from `initialize` |
| Auth | `initialize` handshake: first-time clients are approved interactively by the user (notification); approval issues an access token which clients use for later connections (see 2.5) |
| Concurrency | std threads only, reader/writer thread pair per connection (no tokio, no mio) |
| Events | Type-2 notifications broadcast to all clients (v2: `project_opened`, `project_closed`, `selection_changed`) |
| Scope | GUI-attached only; no headless/CLI support; no Python driver in v2 |
| Docs | Generated Doxygen-flavored Markdown page from the YAML spec (committed); no automated spec check / CI stale-check (tests are written manually) |

Why msgpack instead of JSON: lossless transfer of LibrePCB's low-level
typed data (`Length` as exact i64, `Angle`, `Point`, f64 geometry), binary
blobs without base64 inflation, compactness, and a data model that matches
a strongly typed application. The MCP glue translates values (JSON ↔
msgpack) — accepted cost. `nc`-style debugging is lost and *not* replaced
by debug tooling (explicitly not wanted).

## 2. Protocol specification

### 2.1 Transport & framing

TCP on 127.0.0.1 on the fixed default port 19536 ("LP" = 0x4C50); if it's
already in use, the next higher port is tried (up to 100 attempts —
binding is a cheap syscall, so the fallback logic is negligible). The
Rust server binds the port and starts listening right in its constructor,
there is no separate start step. Messages are **msgpack values decoded
directly from the stream** (self-delimiting, like the official msgpack-rpc
spec): the reader blocks mid-value until more bytes arrive — no length
prefix, no re-parsing of partial data (the decoder consumes each byte
exactly once).

- **Message size cap: 32 MiB, enforced mid-parse** with a counting `Read`
  wrapper that errors once that many bytes were consumed for a single
  value.
- **Decode-level errors** (invalid msgpack, truncated value, cap exceeded)
  desynchronize the stream → the connection is **closed** (no resync
  possible with stream framing).
- **Payload-level errors** (a fully decoded value that doesn't conform:
  wrong envelope shape, non-string map keys, `ext` types, u64 > i64::MAX,
  params not a map, non-string `"obj"`, unknown method, …) keep the stream
  in sync → answered with a **normal RPC error response**; the connection
  stays open. (Proper error handling is intentionally minimal for now and
  will be refined later — the layering above is what must not change.)

### 2.2 Envelope (message types)

msgpack arrays, inspired by the (dormant) official msgpack-rpc spec,
extended with request-scoped message types:

| Type | Message      | Direction | Shape                        | Meaning |
|------|--------------|-----------|------------------------------|---------|
| 0    | Request      | C→S       | `[0, msgid, method, params]` | Call a function |
| 1    | Response     | S→C       | `[1, msgid, error, result]`  | Exactly one per request; `error` is nil or a map; `result` is a map or nil (nil iff error, or if the command has no result) |
| 2    | Notification | S→C       | `[2, method, params]`        | Broadcast event (e.g. `project_opened`) |
| 3    | Request message | S→C   | `[3, msgid, payload]`        | Per-request channel: progress reports and/or streamed partial data; only while the request is pending |
| 4    | Cancel       | C→S       | `[4, msgid]`                 | Client asks to cancel a pending request |

- `msgid` is a u32, unique per connection (per-connection, *not* global).
- Responses may be sent **out of order** (pipelining is allowed, as in the
  official spec): the reader thread keeps reading while requests are being
  processed.
- **Unknown message types are ignored + logged** (not closed) — forward
  compatibility; the set of known types is gated by `protocolVersion`.
- One emit primitive, two conventions: type-3 payloads are defined per
  method in the spec (e.g. progress: `{"percent": <int 0..100>,
  "message": <string>}`, or streamed chunks: `bin` data).
- **Cancellation invariant:** on cancel, the server stops the work *when
  it can* and responds with a spec-defined "cancelled" error; if the work
  already completed or can't be interrupted, the normal result is sent.
  Either way the client receives **exactly one final response** per
  request. The server *enforces* this exactly-once property (see 2.6).

### 2.3 Method naming: `ClassName.method_name`

- Wire name: `"ClassName.method_name"` — classes in `UpperCamelCase`,
  methods in `lower_snake_case` (e.g. `"UndoStack.undo"`,
  `"Project.list"`).
- **Classes are first-class entities in the YAML spec**, each with its
  methods, its description and — for classes whose methods operate on an
  object instance — its **handle type** (see 2.4).
- Static methods (no receiver) and member methods (receiver required) are
  both declared under their class; the spec marks which is which.
- The C++ command surface is generated per spec class (see 4.2a),
  dispatched by the full `"ClassName.method_name"`
  string.
- Event names (type 2) are not class methods; they are listed as separate
  top-level entities in the spec, in `lower_snake_case` (e.g.
  `project_opened`).
- Protocol-level methods (`initialize`, `ping`, see 2.5) are handled by
  Rust and live outside the class scheme.

### 2.4 Params

A request's `params` is a **string-keyed msgpack map**:

- **Reserved key `"obj"`:** the receiver handle for member methods (the
  "this" pointer). For static methods it is absent. An argument may never
  be named `obj`.
- **Handles are opaque:** The handle is an arbitrary value from the
  protocol's value domain. Clients obtain handles from responses
  (embedded under the reserved key `"obj"` in maps which represent
  objects) and must pass them back unchanged, without making any
  assumptions about their type or value — the representation may change
  from command to command and from app version to app version. Neither
  the documentation nor client libraries know anything about the handle
  contents. In the specification file, handles have the special type
  `handle`.
- **`"obj"` is a *documented convention only*** — there is no code for it
  on the Rust side. The whole params map crosses the FFI as one `QVariant`
  (Rust never parses command content at all, not even `obj`); the C++
  dispatch layer extracts `"obj"` and passes a clean `(handle, args)` pair
  to the handlers, where `handle` is an empty string for static methods.
- `obj` present but not a string → payload-level error (detected on the
  C++ side, answered with an RPC error like any other invalid params).
- Non-string map keys anywhere in a payload are payload-level errors too
  (answered, not disconnects) — msgpack allows them, but our boundary
  (`QVariantMap`) and spec are string-keyed, and there is no use case.

### 2.5 Handshake & protocol-level methods

- First message of a connection **must** be an `initialize` request. Any
  other first message → error response (code 30), connection stays open
  but stays unauthenticated. Two ways to authenticate:
  - **With a token:** params `{"token": "<string>"}`. The token must have
    been issued by this application instance (kept in memory, i.e. valid
    for the lifetime of the process). Otherwise → error response (code 31
    `AccessDenied`), connection stays open.
  - **Interactively:** params `{"name": "<client name>"}`. The app shows
    a notification asking the user to accept the connection. On accept,
    the request is replied (success) and a fresh access token is issued
    (returned in the response, to be stored by the client). If the user
    dismisses the prompt or the client cancels the request (type 4), the
    request is replied with `AccessDenied`.
- Success result (a map): `{"format": "1.0", "server": "librepcb",
  "version": "<app version>", "session": "<uuid>", "workspace": "<path>",
  "token": "<token>"}`.
  - `format` is the RPC interface format version (currently `"1.0"`).
    Clients must verify the major version; a higher minor version means
    additive, backward-compatible changes.
  - `session` is a per-process UUID: It changes when the application is
    restarted, so clients can detect that.
  - Clients use `version` + the spec's per-function `since`
    to determine availability: functions added after the revision of the
    client's spec are unknown to the client, which is fine (unknown
    method → RPC error).
- `ping` is a protocol-level method (handled by the Rust server, no
  dispatch): returns `{}`.
- `ping` → nil result.
- **No `rpc.discover`** (dropped): the spec is a file, not a method.

### 2.6 Asynchronous request semantics

- Requests are dispatched to C++ **fire-and-forget** with
  `(conn_id, msgid, method, handle, args)`; the reader thread continues
  immediately (pipelining).
- C++ marshals to the GUI thread with `Qt::QueuedConnection` (no
  blocking!). The handler decides when to answer:
  - immediate result → `ffi_rpc_server_reply(conn_id, msgid, ok, value)`
  - long-running task → zero or more
    `ffi_rpc_server_notify_conn(conn_id, msgid, payload)` (type 3, e.g.
    progress) → reply later
- `conn_id` is needed because msgids are only unique per connection; two
  clients may use the same msgid.
- **Server-side bookkeeping enforces the invariants:** the Rust server
  tracks per connection the set of pending (dispatched, not yet replied)
  msgids. `reply` removes it (a second reply for the same msgid is
  ignored); `notify_conn` (type 3) is only delivered while pending;
  replies/emits to unknown or closed connections are silently ignored
  (covers "task finishes after the client disconnected").
- Cancel `[4, msgid]`: forwarded to C++ via a cancel callback if the msgid
  is pending; C++ stops the work when it can and must still reply exactly
  once (spec-defined "cancelled" error, or the normal result if it already
  completed). Wiring cancellation into real LibrePCB jobs is handler-level
  work, later.
- **Progress can only flow while the GUI event loop pumps:** a handler
  that blocks the GUI thread for minutes cannot report progress. Long
  operations must be event-loop-friendly (chunked state machines or
  background jobs with completion signals — the pattern LibrePCB already
  uses for library scans / DRC).
- Teardown is simple: queued-but-unexecuted GUI invocations die with the
  destroyed `QObject` (Qt drops pending events), pending sets are dropped
  with their connections, late replies are no-ops.

### 2.7 Error codes

The response `error` is a map `{"code": <int>, "message": <string>}` (+
optional extra properties, see the error types in the spec). The codes
are grouped: 10–19 transport, 20–29 protocol, 30–39 command, 100–
command execution. The spec's `errors` section defines code, message
and optional additional properties for each error type.

The canonical `message` of an error code (from the spec) is always
sent on the wire, optionally followed by an application-provided detail
message (separated by a colon): `sendError(code)` → only the canonical
message; `sendError(code, detail)` → `"<canonical>: <detail>"`. The
canonical messages live only in the *generated* Rust code
(`src/rpc_server/rpc_api.rs`, generated by `dev/rpc/gen.py`
from the spec) — the Rust server composes
the wire message during response handling, so C++ does not need to know
the canonical messages.

## 3. rust-core implementation (Qt-free)

### 3.1 Module layout (v2 — rework of the existing v1 code)

```
libs/librepcb/rust-core/src/rpc_server.rs       (module root: RpcServer,
                                                  RpcServerConfig, DispatchFn,
                                                  CancelFn, connection registry
                                                  incl. pending-msgid tracking)
libs/librepcb/rust-core/src/rpc_server/
  protocol.rs        — msgpack envelope: classify/build the 5 message types,
                       token handshake handling (rewritten from JSON-RPC)
  codec.rs           — stream decoding: read one msgpack value from a
                       BufRead (rmpv), 32 MiB counting-Read cap, encode
                       values to bytes (rewritten from NDJSON)
  connection.rs      — accept loop + reader/writer threads (mostly kept
                       from v1; reader loop now feeds the stream decoder
                       instead of read_line)
libs/librepcb/rust-core/src/ffi/rpc_server_ffi.rs   (rewritten: QVariant
                                                boundary, async dispatch;
                                                contains the msgpack↔QVariant
                                                conversion as private helpers)
```

### 3.2 What survives from v1 vs. what is rewritten

- **Kept (conceptually):** threading model (polling accept loop,
  reader/writer pair per connection, non-blocking notify), lifecycle
  (`new` binds immediately with port fallback / `stop` / `Drop` with
bounded thread drain),
  handshake flow, sequential-connection registration, `CppDispatch`
  Send/Sync pattern in the FFI.
- **Rewritten:** envelope (JSON-RPC objects → msgpack arrays), codec
  (NDJSON lines → self-delimiting stream), error layering, FFI (QString
  JSON → QVariant values; sync response out-param → async reply),
  `rpc.discover`/`set_api_schema` **deleted**.
- Direct dependencies: `rmp` + `rmpv` (new), `serde_json` dropped again
  from direct deps.

### 3.3 msgpack ↔ QVariant mapping (restricted subset)

| msgpack | QVariant |
|---|---|
| nil | invalid QVariant |
| bool | `bool` |
| int | `qlonglong` (i64; accepts 32-bit ints too) |
| float | `double` (f32 widens exactly) |
| str | `QString` |
| bin | `QByteArray` |
| array | `QVariantList` |
| map | `QVariantMap` (string keys only) |

- The conversion is **direct** (no intermediate value type!): recursive
  private helpers in `ffi/rpc_server_ffi.rs` walk the msgpack value and
  build the QVariant tree via the FFI setters, and vice versa. The
  `QVariant` concept deliberately exists only at the FFI boundary.
- `ffi_qvariant_type()` returns the **raw Qt metatype ID** — no invented
  type enum. The Rust side (`qmetatype` constants in `ffi/cpp_ffi.rs`,
  kept in sync with the ABI-stable `QMetaType::Type` values) matches the
  IDs of the value domain and rejects everything else.
- u64 values > i64::MAX, `ext` types, non-string map keys and invalid
  UTF-8 strings → payload-level errors ("invalid params").
- The C++ dispatch layer *normalizes* command results into the domain
  (plain `int`/`uint`/`float` are converted, anything else like
  `QDateTime` is rejected with an internal error).
- The conversion helpers are implemented in `ffi.cpp` (~20 one-liners:
  allocate/free, setters for each scalar + bytearray, list/map builders,
  type query + readers). (The `qstring_*` helpers already exist;
  `qvariant_*` are added alongside.)

### 3.4 FFI surface (cbindgen, opaque handle)

```rust
/// Async command dispatch: called on the connection's reader thread.
/// No response parameter — the reply is sent later via
/// ffi_rpc_server_reply(). `params` is the whole params map (including
/// the reserved "obj" key, which is extracted by the C++ dispatch layer
/// — it is a documented convention, not Rust code).
pub type RpcDispatchFn = extern "C" fn(
    ctx: *mut c_void, conn_id: u64, msgid: u32,
    method: &QString, params: &QVariant);

/// Cancel notification: a client sent [4, msgid] for a pending request.
/// C++ must eventually reply exactly once (cancelled error or result).
pub type RpcCancelFn = extern "C" fn(ctx: *mut c_void, conn_id: u64, msgid: u32);

ffi_rpc_server_new(token: &QString, protocol_version: &QString,
                   info: &QVariantMap-as-QVariant, dispatch: RpcDispatchFn,
                   cancel: RpcCancelFn, ctx: *mut c_void) -> *mut RpcServer;
ffi_rpc_server_start(s: &mut RpcServer, port: u16, out_port: &mut u16) -> bool;
/// Reply to a request (exactly-once enforced server-side). ok=true: value
/// is the result. ok=false: value is the error map — which is forwarded to
/// the client *unchanged*, so it may carry additional error properties
/// next to "code" and "message" (see the error types in the API spec).
/// No return value: If the connection/msgid is unknown or already replied,
/// the response is silently dropped; if the value is not convertible, an
/// internal error is sent instead (the caller does not care).
ffi_rpc_server_reply(s: &mut RpcServer, conn_id: u64, msgid: u32,
                     ok: bool, value: &QVariant);
/// Broadcast an event (type 2) to all clients; never blocks.
ffi_rpc_server_notify_all(s: &mut RpcServer, method: &QString, params: &QVariant);
/// Send a request-scoped message (type 3, e.g. progress) while the
/// request is pending; returns false otherwise.
ffi_rpc_server_notify_conn(s: &mut RpcServer, conn_id: u64, msgid: u32,
                           payload: &QVariant) -> bool;
ffi_rpc_server_delete(s: *mut RpcServer);
```

Notes:
- All reply/notify functions are non-blocking (unbounded channel push to
  the writer threads) and thus safe to call from the GUI thread.
- The `CppDispatch` newtype pattern (wrapping fn-pointer + ctx to make
  them `Send`/`Sync`, called through a helper function to avoid disjoint
  closure captures) is kept from v1.
- Rust never parses command content: only the envelope (message type,
  msgid, method string) — the whole params map crosses as QVariant
  opaquely (the msgpack ↔ QVariant conversion lives in the FFI layer).

## 4. C++ implementation

### 4.1 Files

```
libs/librepcb/editor/rpc/rpcserver.{h,cpp}    (hand-written)
libs/librepcb/editor/rpc/rpcrequest.{h,cpp}   (hand-written)
libs/librepcb/editor/rpc/rpcerror.h          (hand-written error base class)
libs/librepcb/editor/rpc/rpcerrors.h         (generated error classes)
libs/librepcb/editor/rpc/projectrpc.{h,cpp}   (.h generated, .cpp hand-written)
libs/librepcb/editor/rpc/undostackrpc.{h,cpp} (.h generated, .cpp hand-written)
libs/librepcb/editor/rpc/workspacedbrpc.{h,cpp} (ditto)
libs/librepcb/editor/rpc/commands.cpp         (100% generated dispatcher)
```

### 4.2 `RpcServer` (QObject, created by `GuiApplication`)

- **Lifecycle:** started/stopped **reactively** according to the
  workspace setting `enable_rpc_server` (see 4.4); torn down early in
  `GuiApplication`'s destructor.
- **Dispatch:** the FFI trampoline runs on Rust connection threads and
  marshals to the GUI thread with `QMetaObject::invokeMethod(...,
  Qt::QueuedConnection)` carrying `(conn_id, msgid, method, args)` —
  **no blocking, no deadlock hazard** (v1's `BlockingQueuedConnection`
  design and its deadlock rule are obsolete). The trampoline itself
  extracts the `"obj"` key from the params map (documented convention)
  and rejects non-string values; unknown methods are answered with a
  `-32601` error reply directly from the reader thread (the reply path is
  thread-safe).
- **Request context:** handlers receive `RpcRequest` (see 4.2a), which
  does *not* hold references to the application or the server anymore —
  it only wraps `(conn_id, msgid, method, args)` plus the reply/cancel
  signals. Handlers get the application as an explicit parameter.
- **Exceptions:** all exceptions are caught in `handleRequest` and
  converted to `CommandFailed` error replies — exceptions never cross
  the FFI. The generated dispatcher stays exception-neutral.
- **Command dispatch (generated):** see 4.2a below; there is no runtime
  command registry with lambdas anymore.
- **Events:** connect existing `GuiApplication`/`ProjectEditor` signals →
  `ffi_rpc_server_notify_all(...)` (type 2), e.g. `project_opened`,
  `project_closed`, `selection_changed`.
- **Cancel:** the cancel callback looks up the request context registry on
  the GUI thread (queued) and cancels if the handler supports it; minimal
  in v2 (ignore + still reply, e.g. with the result or a cancelled error).

### 4.2a Generated command surface (per spec class)

The C++ command surface is **generated from the spec** by `dev/rpc/gen.py`
(like the docs and `rpc_api.rs`; generated files are committed):

- For each class in the spec, a header
  `libs/librepcb/editor/rpc/{classname}rpc.h` with a static-only class
  `{ClassName}Rpc` (default constructor deleted → no instantiation):
  one `static void {method}(GuiApplication& app, RpcRequest& request)`
  per spec method, with the spec description as doc comment. Static vs.
  non-static (`obj` handle) does not change the generated signature —
  handle extraction stays in the hand-written implementation (e.g. the
  `findProject()` helper lives in `projectrpc.cpp`).
- One generated file `commands.cpp` (no header, 100% generated): defines
  the free function
  `void rpcDispatchCommand(GuiApplication& app, const QString& method,
  RpcRequest& request)`, which looks the method up in a static
  function-pointer map (`{"Project.list", &ProjectRpc::list}`, ...) —
  defined right inside the dispatcher function — and calls it, passing
  the application. Unknown methods are reported by throwing
  `RpcMethodNotFound` (see 4.2b).
- The declaration (with a short comment) of `rpcDispatchCommand()` is
  hand-written in `rpcserver.cpp`; `handleRequest` calls it inside its
  try/catch and replies `MethodNotFound` if it returns `false`.
- The `.cpp` implementations of the command methods are **hand-written**
  (the generator does not create them; they are created manually during
  implementation). `initialize`/`ping` remain protocol-level methods
  handled by `RpcServer` itself (initialize needs `mHandle`).
- Handlers take `(GuiApplication& app, RpcRequest& request)` — non-const
  app, since commands mutate application state (e.g. `openProject()`).
  If a handler needs nothing but the request, it just ignores the app.

### 4.2b Error handling (exceptions)

- Hand-written base class `rpcerror.h`: `RpcError : std::exception`,
  wrapping `(code, detail, properties)`; `what()` returns the detail.
- Generated `rpcerrors.h` (by `gen.py`, from the spec): the error code
  enum `RpcErrorCode : int` — **replacing the cbindgen-exported enum**,
  so no `*.h` file needs to include `ffi.h` for the codes anymore (both
  the Rust and the C++ enum are generated from the same spec) — plus
  one exception class per error code (`RpcInvalidHandle`,
  `RpcCommandFailed`, ...), each final and fixing its code in its
  constructor.
- Command implementations just *throw* (e.g. `throw RpcInvalidHandle();`
  or `throw RpcCommandFailed("The project file does not exist.");`) —
  no `sendError()` calls and no early returns needed.
  `RpcServer::handleRequest` contains the single catch ladder:
  `RpcError` → its code and detail; `std::exception` → `CommandFailed`
  with `what()` as detail; catch-all → `CommandFailed`.
- Asynchronous errors (a handler which replies later) use the same
  classes, but the continuation catches and calls
  `request.sendError(e)` itself — there is no framework catch on the
  GUI event loop, so exceptions must never escape into it.
- Helper functions like `findProject()` throw instead of returning
  nullptr.

### 4.3 v2 command set (illustrative mapping from v1)

| v1 name | v2 name | Kind |
|---|---|---|
| `project/list` | `Project.list` | static |
| `project/get` | `Project.get` | member (obj = project UUID) |
| `undostack/getStatus` | `UndoStack.get_status` | member (obj = project UUID) |
| `undostack/undo` | `UndoStack.undo` | member |
| `undostack/redo` | `UndoStack.redo` | member |

(The handle type of `UndoStack` is the UUID of the project whose editor
owns the stack — documented per class in the spec. The final API surface
is designed during implementation; classes are expected to grow, e.g.
`Schematic`, `Board`, `Library`.)

All mutating handlers push proper `UndoCommand`s onto the project's
`UndoStack` (mandatory per AGENTS.md) — remote edits appear in the user's
undo history; the GUI stays the single source of truth.

### 4.4 Workspace setting

`enable_rpc_server` (default off), following the existing
`WorkspaceSettingsItem_GenericValue<bool>` pattern. The settings-dialog
checkbox is still missing (to be added when the feature becomes
user-facing, together with the random token).

The GuiApplication starts/stops the server **reactively**: an
`updateRpcServerEnabled` lambda (connected to the setting's `edited`
signal) starts the server when the setting turns on and stops it when it
turns off — i.e. the server can be toggled at runtime from the settings
dialog, without restarting the application. There is no env var
fallback.

For the system tests, the setting is enabled in the **demo workspace
settings file** (`tests/data/workspaces/Empty Workspace/data/settings.lp`
— note that the settings file the application reads lives in the *data*
directory, not in the version directories). The server listens on its
fixed default port 19536 ("LP" = 0x4C50; no test env var for the port
anymore).

## 5. API spec (YAML) & documentation

### 5.1 Spec file

`dev/rpc/rpc.yml` — the single source of truth for the API. It lives
in the dev tools (not shipped with the app); local tools read the spec
matching their installed server. Clients compare per-function
`since` with the `version` from `initialize`.

Results are **map-or-nil**: A method's `result` is either a struct
name or the list of the map's fields; an empty field list (or null)
declares that the command returns nil on the wire.

Structure sketch:

```yaml
spec_version: 1
protocol_version: 1

enums:
  - name: GridStyle            # illustrative
    description: ...
    values:
      - name: dots             # wire value = the string itself
        description: ...

errors:
  - name: InvalidHandle
    code: 35
    message: "Invalid object handle"

classes:
  - name: Project
    description: An open project
    handle: An opaque object handle (embedded in the responses under the "obj" key).
    methods:
      - name: list             # static → no obj
        static: true
        description: List all open projects
        params: []
        result:
          - name: projects
            type: array
            items: ProjectSummary
            description: All open projects
        since: "2.2.0"
      - name: get              # member → obj = the project handle
        description: Get project details
        params: []
        result: ProjectDetails
        since: "2.2.0"

events:
  - name: project_opened
    description: A project was opened
    params: {type: ProjectSummary}
    since: "2.2.0"

# + top-level type definitions (structs) for reuse in params/results
```

- Types: `string`, `int` (i64), `float` (f64), `bool`, `binary`, `null`,
  `array`, `map`, enum/struct references. Domain numeric conventions are
  documented on the fields (e.g. "length in nanometers").
- **Enums are strings on the wire**; the spec lists the allowed values and
  is the validation reference. Unknown enum string → payload-level error.
- Per-function `since`: compared against `appVersion`
  (suffix-tolerant semver comparison, e.g. "2.1.2-unstable" → "2.1.2").
- **No automated spec-vs-implementation check** (deliberately dropped;
  tests are written manually).

### 5.2 Code & docs generation

The dev tooling lives in `dev/rpc/` (a uv project, see `pyproject.toml`
there; run the generator with `uv run gen.py`):

- `rpc.yml` — the specification (see 5.1).
- `templates/` — Jinja2 templates for the generated files.
- `gen.py` — generates, from the spec and the templates:
  - `dev/doxygen/pages/rpc_api.md` — the generated API reference (data
    types, error types, classes with their methods and events).
  - `src/rpc_server/rpc_api.rs` — generated Rust code (for now only the
    error codes with their canonical messages).

Both files are committed, so the build needs no Python. The transport
protocol is a separate hand-written page
(`dev/doxygen/pages/rpc_server.md`) which embeds the reference via
Doxygen's `\subpage` mechanism. Later, `gen.py` will also validate the
specification. No CI stale-check.

## 6. Testing

- **rust-core** (`cargo test`, without `ffi` feature): envelope
  (de)serialization for all 5 message types, stream codec edge cases
  (partial reads across chunk boundaries, 32 MiB cap, invalid msgpack →
  disconnect), handshake/auth, async dispatch/reply/emit bookkeeping
  (exactly-once, pending tracking, late replies ignored, per-connection
  msgid scoping), notify broadcast, cancel forwarding. The
  msgpack↔QVariant conversion lives in the FFI module (only compiled with
  the `ffi` feature) and is covered by the system test instead.
- **C++ unittests:** none for now (the user writes command tests
  themselves); the dispatch layer's normalization could be unit-tested
  later if it grows.
- **Python system test** (`tests/ui/rpc/`, *not* funq — funq tests are
  legacy): launch the app (the demo workspace settings enable the RPC
  server) with a pinned port (`LIBREPCB_RPC_PORT`), connect a msgpack
  socket client from the test, `initialize`, call commands, assert
  results — and thus implicitly verify the whole msgpack↔QVariant
  conversion, the async dispatch and the GUI-thread marshalling. The test
  uses the `u-msgpack-python` package for the MessagePack encoding.

## 7. Milestones

1. ~~v1: rust-core JSON-RPC server + cargo tests~~ **done, superseded.**
2. ~~v1: FFI + C++ `RpcServer` skeleton + `api.json` + docs pipeline~~
   **done, superseded.**
3. **Protocol v2 in rust-core** — `rmp`/`rmpv` deps; new `protocol.rs`
   (types 0–4, handshake), `codec.rs` (self-delimiting stream + 32 MiB
   cap), async dispatch with reply/emit/cancel bookkeeping; rewrite the FFI
   module; delete `rpc.discover`/`set_api_schema`; cargo tests.
   **Status: Done.** 166 Rust tests pass, clippy clean (the FFI module
   converts msgpack ↔ QVariant directly, without an intermediate value
   type — the conversion rules are covered by the system test).
4. **Protocol v2 C++ integration** — async `RpcServer` (QueuedConnection,
   request context), QVariantMap handlers, `rpc.yml` (classes `Project`,
   `UndoStack`; enums/error types as needed), docs pipeline (hand-written
   protocol page + generated reference page), Python system test in
   `tests/ui/rpc/`.
   **Status: Done.** Verified: 2606 C++ unittests pass, and the system
   test passes against the real application (with an open project,
   covering handshake, ping, `Project.list`/`get`, the full
   `UndoStack` API, unknown methods, invalid envelopes, value-domain
   rejections, error properties, unknown-cancel and pipelining).
   No C++ command unit tests (the user writes them themselves), and the
   async marshalling means there are no more blocking-dispatch hazards.
5. **Events + progress/cancel end-to-end** — type-2 broadcasts from
   existing signals; a long-running command emitting type-3 progress;
   cancel flow with exactly-once reply.
6. **`apps/librepcb-mcp`** (separate PR) — stdio MCP server: value
   translation (JSON ↔ msgpack), token, tool surface derived from the
   spec.
7. **Later (additive):** WebSocket transport (first-class endpoint or a
   proxy plugin — to be decided), Python driver, instance registry /
   `ProcessFile` (section 9), async client libraries.

## 8. Risks / gotchas

- **Every accepted request must eventually be replied** (or the client
  waits forever): a hung handler is a client-side timeout problem, but
  handlers must be written to always answer — including on all error
  paths (the dispatch layer's catch-all covers exceptions).
- **Progress requires a pumping event loop** — blocking the GUI thread
  for minutes means no progress; long tasks must be event-loop-friendly.
- **Stream framing has no resync:** decode-level garbage closes the
  connection. Payload-level garbage is answered with an RPC error and
  tolerated (this distinction is load-bearing — see 2.1).
- **Exactly-once replies are enforced server-side** (pending-msgid
  tracking), not just by convention — duplicate replies and emits after
  completion are silently dropped.
- **msgid scoping is per connection** — C++ must store the `conn_id` with
  every pending request.
- **Hardcoded token is development-only:** any local process can connect.
  Must be replaced by the random token + instance registry before
  shipping (section 9).
- **Sandboxing (Flatpak/Snap):** TCP on 127.0.0.1 works because sandboxes
  share the host network namespace — *if* network permissions are granted
  (Flatpak `share=network`; Snap `network` + `network-bind` plugs —
  packaging note). The future instance file must live in
  `QStandardPaths::RuntimeLocation` (the only sandbox-proof location);
  the recorded PID is sandbox-internal and informational only; liveness =
  connect + handshake. For web clients (if WS ever happens): browsers
  allow localhost WS connections from *any* website, making the token
  essential — and mixed content only allows it from `http://localhost` /
  `file://` pages.
- cbindgen must handle the `RpcDispatchFn`/`RpcCancelFn` typedefs and the
  opaque `RpcServer` handle (already proven in v1).
- Keep rust-core Qt-free (AGENTS.md): the `rpc_server` module uses only
  std + `rmp`/`rmpv` (QVariant interop lives in the FFI layer, like the
  QString helpers).
- Lost debuggability vs JSON (accepted): msgpack dumps need a decoder;
  no debug CLI will be built.

## 9. Deferred design: `ProcessFile` (instance registry)

The instance registry was deferred to keep the RPC server feature simple
and straightforward. Interim state: **hardcoded token** (development
only, see risks). The following is the agreed design for the follow-up.

### Concept

- A generic Rust module `ProcessFile` (or similar) in rust-core, not tied
  to `rpc_server`:
  - `new(...)`: Creates `<$XDG_RUNTIME_DIR>/librepcb/instances/<uuid>.json`
    with the instance data, plus an empty lock file at the same path with
    an additional `.lock` suffix, locked with `std::fs::File::lock()` for
    the holder's lifetime. The JSON content comes from a Rust struct which
    is filled from C++ and passed to `new()`.
  - `update(data)`: Atomically replaces the JSON file (write to a
    temporary file + rename) with the new content.
  - `Drop`: Deletes both files. C++ calls `new()` and stores the handle in
    a smart pointer / RAII wrapper which automatically calls drop on free.
- Where the filename UUID comes from does not matter — possibly passed as
    a string to `new()`, coming from C++ for now (e.g. `QUuid::createUuid()`),
    maybe replaced by the `uuid` crate later.
- The RPC token is part of the ProcessFile data structure, filled by C++
  for now. The RPC server receives the token when it is started — it does
  not need to know that the token comes from a ProcessFile. The token is
  considered an *opaque string*; whether it is a UUID or something else
  is an implementation detail.
- Clients can (optionally) use the lock file for stale file detection
  (process crashed → the OS released the lock → a client's `try_lock()`
  succeeds). A heartbeat (updating the JSON file with a timestamp at
  regular intervals) is an alternative — to be decided later.
- Base directory: `$XDG_RUNTIME_DIR` on Linux (see section 8 for why this
  is the only sandbox-proof location). *Open point:* whether `ProcessFile`
  resolves the base directory itself (reads `XDG_RUNTIME_DIR` / temp dir)
  or receives it from C++ (`QStandardPaths::RuntimeLocation`, matching the
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

### Server verification (follow-up)

The current design leaves the client unable to authenticate the server:
an attacker (other local user) can squat the RPC port before the app
starts and harvest an access token by relaying the `initialize` request
to the real app (the user approves the prompt, believing it's their own
client), or impersonate the app. To be fixed with:

- The instance file carries a random per-process `secret` (file mode 0600,
  so other users cannot read it).
- `initialize` becomes two-phase: phase 1 carries a fresh client `nonce`
  and the server response proves knowledge of the secret with
  `HMAC-SHA256(secret, nonce)`; the client disconnects on mismatch. The
  token is only sent in phase 2, on a verified connection — phase 2 is
  the existing token/approval logic, unchanged.

Details (file layout, fallbacks, tests, GUI tool) to be worked out when
implementing. Until then, clients connecting by port are unsafe in
multi-user environments.
