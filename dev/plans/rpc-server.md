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
│                              │ replies / progress (types 1,2) │  non-block.) │
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
| Method naming | `ClassName/method_name` (classes are first-class entities in the spec; see section 2.3) |
| Params | String-keyed map with a reserved `"obj"` key carrying the receiver handle — an *opaque* value (see section 2.4) |
| Domain types | Plain integers (`Length` in nm, `Angle` in µdeg, …) and f64 — **forever**, no msgpack `ext` types |
| Enums | Strings (self-describing; clients don't need the spec to display them) |
| Spec / discovery | YAML spec file (`rpc/api.yml`, dev tool, not shipped); **no `rpc.discover` method** — clients read the spec file and compare per-function `since` with the `version` from `initialize` |
| Auth | `initialize` handshake: first-time clients are approved interactively by the user (notification); approval issues an access token which clients use for later connections (see 2.5) |
| Concurrency | std threads only, reader/writer thread pair per connection (no tokio, no mio) |
| Events | Reserved: msgid-0 requests as notifications, broadcast by the application (e.g. `project_opened`) — not yet used |
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
  wrong envelope shape, non-string map keys, `ext` types,
  params not a map, non-string `"obj"`, unknown method, …) keep the stream
  in sync → answered with a **normal RPC error response**; the connection
  stays open. (Proper error handling is intentionally minimal for now and
  will be refined later — the layering above is what must not change.)

### 2.2 Envelope (message types)

msgpack arrays, inspired by the (dormant) official msgpack-rpc spec,
extended with request-scoped message types. The protocol is
*bidirectional*: Both sides can initiate calls in exactly the same way
(e.g. the application calling methods provided by a plugin), with the
following message types:

| Type | Message      | Direction     | Shape                        | Meaning |
|------|--------------|---------------|------------------------------|---------|
| 0    | Request      | both          | `[0, msgid, method, params]` | Call a function |
| 1    | Response     | callee→caller | `[1, msgid, status, payload]`  | Exactly one per request; `status` is 0 on success (payload = result) or the error code (payload = error map) |
| 2    | Notification | callee→caller | `[2, msgid, payload]`        | Progress and/or streamed (partial) result data, merged into a rolling result (maps recursively, arrays appended, everything else overwritten); only while the request is pending (see the protocol docs) |
| 3    | Cancel       | caller→callee | `[3, msgid]`                 | The caller asks to cancel a pending request |

- `msgid` is a u32, **unique per direction while pending** (a response
  always travels in the opposite direction of its request, so msgids of
  concurrent requests in both directions never collide).
- The **msgid 0 is reserved** for future notifications, i.e. method calls
  the sender is not interested in a response for (which MUST NOT be
  replied to). Senders must not use msgid 0 yet, and it is rejected as an
  invalid request. Malformed requests (e.g. with an undeterminable
  msgid) are answered with an error response to msgid 0.
- Responses may be sent **out of order** (pipelining is allowed, as in the
  official spec): the reader thread keeps reading while requests are being
  processed.
- **Unknown message types are ignored + logged** (not closed) — forward
  compatibility.
- One emit primitive: The notification payload carries the reserved
  state members (`progress`, `step`, `steps`, `message`) and/or a
  `result` member with partial result data, which is merged into a
  rolling result on the client side (maps recursively, arrays appended,
  everything else — including nil — overwritten; no deletion
  semantics) — the final response payload merges into it as well and
  only carries data not delivered yet. Consequently the response alone
  is the complete result if no `result` fragment was received.
  Notifications are only sent while the request is pending; progress
  is not monotonic and never a completion signal.

### 2.3 Method naming: `ClassName/method_name`

- Wire name: `"ClassName/method_name"` — classes in `UpperCamelCase`,
  methods in `lower_snake_case` (e.g. `"UndoStack/undo"`,
  `"ProjectEditor/list"`).
- **Classes are first-class entities in the YAML spec**, each with its
  methods, its description and — for classes whose methods operate on an
  object instance — its **handle type** (see 2.4).
- Static methods (no receiver) and member methods (receiver required) are
  both declared under their class; the spec marks which is which.
- The C++ command surface is generated per spec class (see 4.2a),
  dispatched by the full `"ClassName/method_name"`
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

- Protocol versions are monotonic integers starting at **1**. Every
  version N is a cumulative superset of all versions < N; dropping
  features is expressed by raising the oldest still-supported version
  (the server serves the supported range in parallel, so this only
  breaks clients older than that range).
- First message of a connection **must** be an `initialize` request, the
  second an `authenticate` request. Any other message while
  unauthenticated → error response (code 30), connection stays open.
- `initialize` params `{"protocol": 1}` (the client's highest supported
  protocol version). Success result: `{"protocol": <negotiated>,
  "nonce": "<string>"}`.
  - The negotiated version is `min(client_protocol, server_protocol)`,
    clamped to the range of versions the server still supports; the
    server never rejects on version grounds. A client which cannot speak
    the negotiated version closes the connection locally.
  - The `initialize` structure is frozen forever.
  - `nonce` is a fresh per-connection challenge.
  - Host verification is not part of the protocol yet (clients transmit
    nothing reusable); a later version may add it additively.
- `authenticate` params `{"name": "<string>", ...}` with one of:
  - **External client:** `{"pubkey": "<hex>", "proof": "<sig over nonce>"}`
    (Ed25519). Unknown key → interactive confirmation; the user's
    acceptance is remembered, so later connections are silent. Known key
    → silent. Invalid proof → `AccessDenied`.
  - **Spawned process** *(not implemented yet)*: `{"token": "<out-of-band
    token>"}` (no crypto needed). Valid only for the lifetime of the
    issuing instance.
  - Success result: `{"server": "librepcb", "version": "<app version>",
    "session": "<uuid>"}`. The reply authenticates the connection.
  - `session` is a per-process UUID: It changes when the application is
    restarted, so clients can detect that.
  - Clients use `version` + the spec's per-function `since`
    to determine availability: functions added after the revision of the
    client's spec are unknown to the client, which is fine (unknown
    method → RPC error).
- **Endpoint discovery** *(planned, not implemented yet)*: the server
  shall write `{pid, port, session}` to a 0600 file in
  `$XDG_RUNTIME_DIR` at startup; clients connect to the port from that
  file and may verify the `session` to confirm they reached exactly the
  process which wrote it.
- `ping` is a protocol-level method, but it is dispatched to the C++
  handler like any other command (and thus requires authentication):
  returns nil.
- **No `rpc.discover`** (dropped): the spec is a file, not a method.

### 2.6 Asynchronous request semantics

- Requests are dispatched to C++ **fire-and-forget** with
  `(conn_id, msgid, method, handle, args)`; the reader thread continues
  immediately (pipelining).
- C++ marshals to the GUI thread with `Qt::QueuedConnection` (no
  blocking!). The handler decides when to answer:
  - immediate result → `ffi_rpc_server_reply(conn_id, msgid, ok, value)`
  - long-running task → zero or more
    `ffi_rpc_server_notify_conn(conn_id, msgid, payload)` (type 2, e.g.
    progress) → reply later
- `conn_id` is needed because msgids are only unique per connection; two
  clients may use the same msgid.
- **Server-side bookkeeping enforces the invariants:** the Rust server
  tracks per connection the set of pending (dispatched, not yet replied)
  msgids. `reply` removes it (a second reply for the same msgid is
  ignored); `notify_conn` (type 2) is only delivered while pending;
  replies/emits to unknown or closed connections are silently ignored
  (covers "task finishes after the client disconnected").
- Cancel `[3, msgid]`: forwarded to C++ via a cancel callback if the msgid
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

An error response is a response whose `status` carries the numeric error
code; its payload is then a map `{"message": <string>}` plus optional
extra properties (see the error types in the spec) — the code is *not*
part of the payload. The codes are grouped: 10–19 transport, 20–29
protocol, 30–39 command, 100– command execution. The spec's `errors`
section defines code, message and optional additional properties for
each error type.

The canonical `message` of an error code (from the spec) is always
sent on the wire, optionally followed by an application-provided detail
message (separated by a colon): `sendError(code)` → only the canonical
message; `sendError(code, detail)` → `"<canonical>: <detail>"`. The
canonical messages live only in the *generated* Rust code
(`src/rpc_server/rpc_api.rs`, generated by `rpc/gen.py`
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
  protocol.rs        — msgpack envelope: classify/build the message types
                       (0–3), handshake handling (rewritten from JSON-RPC)
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
- Ext values (the API does not use any, and the FFI rejects them),
  non-string map keys and invalid UTF-8 strings →
  payload-level errors ("invalid params"). Large u64 values are
  supported (stored as `QVariant::ULongLong`).
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

/// Cancel notification: the client sent [3, msgid] for a pending request.
/// C++ must eventually reply exactly once (cancelled error or result).
pub type RpcCancelFn = extern "C" fn(ctx: *mut c_void, conn_id: u64, msgid: u32);

ffi_rpc_server_new(token: &QString, protocol_version: &QString,
                   info: &QVariantMap-as-QVariant, dispatch: RpcDispatchFn,
                   cancel: RpcCancelFn, ctx: *mut c_void) -> *mut RpcServer;
ffi_rpc_server_start(s: &mut RpcServer, port: u16, out_port: &mut u16) -> bool;
/// Reply to a request (exactly-once enforced server-side). ok=true: value
/// is the result. ok=false: value is the error map — which is forwarded to
/// the client *unchanged*, so it may carry additional error properties
/// next to "message" and other members of the error type (see the error
/// types in the API spec; the code itself is passed as the response
/// status). The canonical error message is composed by the Rust server,
/// so C++ only provides the application-specific detail.
/// No return value: If the connection/msgid is unknown or already replied,
/// the response is silently dropped; if the value is not convertible, an
/// internal error is sent instead (the caller does not care).
ffi_rpc_server_reply(s: &mut RpcServer, conn_id: u64, msgid: u32,
                     status: u32, ok: bool, value: &QVariant);
/// Broadcast an event (type 2) to all clients; never blocks.
/// (Planned for application events like `project_opened`; not implemented
/// yet.)
ffi_rpc_server_notify_all(s: &mut RpcServer, method: &QString, params: &QVariant);
/// Send a request-scoped message (type 2, e.g. progress) while the
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
libs/librepcb/editor/rpc/rpcobjectregistry.{h,cpp} (hand-written)
libs/librepcb/editor/rpc/api/rpcerrors.h     (generated error classes)
libs/librepcb/editor/rpc/api/projecteditorrpc.{h,cpp}   (.h generated, .cpp hand-written)
libs/librepcb/editor/rpc/undostackrpc.{h,cpp} (.h generated, .cpp hand-written)
libs/librepcb/editor/rpc/workspacedbrpc.{h,cpp} (ditto)
libs/librepcb/editor/rpc/api/commands.cpp         (100% generated dispatcher)
```

### 4.2 `RpcServer` (QObject, created by `GuiApplication`)

- **Lifecycle:** started/stopped **reactively** according to the
  workspace setting `enable_rpc_server` (see 4.4); torn down early in
  `GuiApplication`'s destructor.
- **Dispatch:** the FFI trampoline runs on Rust connection threads and
  marshals to the GUI thread with `QMetaObject::invokeMethod(...,
  Qt::QueuedConnection)` carrying `(conn_id, msgid, method, args)` —
  **no blocking, no deadlock hazard** (v1's `BlockingQueuedConnection`
  design and its deadlock rule are obsolete). The params map is passed
  through opaquely; the reserved `"obj"` key is read by the command
  handlers themselves (see 4.2c), and unknown methods are reported by
  the generated dispatcher throwing `RpcMethodNotFound` (see 4.2b).
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
  `project_closed`, `selection_changed`. *(Planned; not implemented yet.)*
- **Cancel:** the cancel callback looks up the request context registry on
  the GUI thread (queued) and cancels if the handler supports it; minimal
  in v2 (ignore + still reply, e.g. with the result or a cancelled error).

### 4.2a Generated command surface (per spec class)

The C++ command surface is **generated from the spec** by `rpc/gen.py`
(like the docs and `rpc_api.rs`; generated files are committed):

- For each class in the spec, a header
  `libs/librepcb/editor/rpc/api/{classname}rpc.h` with a static-only
  class `{ClassName}Rpc` (default constructor deleted → no
  instantiation): one
  `static void {method}(GuiApplication& app, RpcObjectRegistry& registry,
  RpcRequest& request)` per spec method, with the spec description as
  doc comment. Static vs. non-static (`obj` handle) does not change the
  generated signature — handle extraction stays in the hand-written
  implementation.
- One generated file `rpc/api/rpctypes.h`: the spec enums as C++ enum
  classes, with generated argument readers (per-enum traits with the
  string→value mapping and the enum name — consumed by the generic
  request members `getArg<E>(name)` / `getOptionalArg<E>(name)`;
  invalid values throw `RpcInvalidArgs`). The protocol version is
  generated here as well (`kProtocolVersion`, see 2.5).
- One generated file `commands.cpp` (no header, 100% generated): defines
  the free function
  `void rpcDispatchCommand(GuiApplication& app, const QString& method,
  RpcRequest& request)`, which looks the method up in a static
  function-pointer map (`{"ProjectEditor/list", &ProjectEditorRpc::list}`, ...) —
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

### 4.2c Object handles (registry)

Handles are **identity-based, not value-based**: a handle is an opaque
number referring to a registered QObject, so it survives renames, moves
and UUID changes of the object, and it dies with the object (no
stale-handle traps like UUID or file path handles would have).

- `rpc/rpcobjectregistry.{h,cpp}` (framework): `RpcObjectRegistry`, a
  QObject member of `RpcServer` — its lifetime is the server's, so a
  server restart clears all handles, while the static, monotonic handle
  counter survives restarts so handle values are never reused.
- Registration is lazy at serialization time (bidirectional maps → the
  same object always yields the same handle; `destroyed()` removes the
  entries automatically). Handle 0 is reserved (null). On the wire,
  integers are encoded minimally by the msgpack encoder, so handles and
  other small numbers are as compact as possible (the value domain is
  i64, but that only defines the guaranteed round-trip range, not the
  encoding).
- The registry is passed to the command handlers directly (as generated
  dispatcher parameter). Typed lookup: `getAs<T>(const QVariant& handle)`
  returns a `T&` and throws `RpcInvalidHandle` if the handle is unknown
  or of the wrong type (`qobject_cast`) — so handlers are one-liners.
- GUI-thread only (no locking needed).

The response payload and enumeration conventions (summary vs. details,
one enumeration point per collection, depth via classes, ...) are
documented in the doxygen guidelines page (see 5.2).

### 4.3 v2 command set (illustrative mapping from v1)

| v1 name | v2 name | Kind |
|---|---|---|
| `project/list` | `ProjectEditor/list` | static |
| `project/get` | `ProjectEditor.get` | member (obj) |
| `undostack/getStatus` | `UndoStack/get` | member (obj) |
| `undostack/undo` | `UndoStack.undo` | member |
| `undostack/redo` | `UndoStack.redo` | member |

(Handles are opaque values, embedded in responses under the reserved
"obj" key — see the schema notes in the spec. The final API surface
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

`rpc/api.yml` — the single source of truth for the API. It lives
in the dev tools (not shipped with the app); local tools read the spec
matching their installed server. Clients compare per-function
`since` with the `version` from `initialize`.

Results are **map-or-nil**: A method's `result` is either a struct
name or the list of the map's fields; an empty field list (or null)
declares that the command returns nil on the wire.

Structure sketch:

```yaml
format: "0.1"      # The format of this yml file itself ("MAJOR.MINOR").
protocol: 1        # The wire protocol version described by this file.

enums:
  - name: GridStyle            # illustrative
    description: ...
    values:
      - name: dots             # wire value = the string itself
        description: ...

errors:
  - name: InvalidHandle
    code: 41
    message: "Invalid object handle"

classes:
  - name: ProjectEditor
    description: An open project
    methods:
      - name: list             # static → no obj
        static: true
        description: List all open projects
        params: []
        result:
          - name: projects
            type: array
            items: ProjectEditorSummary
            description: All open projects
        since: "2.2.0"
      - name: get              # member → obj = the project handle
        description: Get project details
        params: []
        result: ProjectEditorDetails
        since: "2.2.0"

events:                        # (reserved, not implemented yet)
  - name: project_opened
    description: A project was opened
    params: {type: ProjectEditorSummary}
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

The dev tooling lives in `rpc/` (a uv project, see `pyproject.toml`
there; run the generator with `uv run gen.py`):

- `rpc.yml` — the specification (see 5.1).
- `templates/` — Jinja2 templates for the generated files.
- `gen.py` — generates, from the spec and the templates:
  - `dev/doxygen/pages/rpc_api.md` — the generated API reference (data
    types, error types, classes with their methods and events).
- `dev/doxygen/pages/rpc_guidelines.md` — **hand-written** guidelines for
    RPC interface developers (payload/enumeration conventions, handler
    rules, error handling, object handles). The conventions live here
    (moved out of this plan) and are referenced from sections 4.2/4.3.
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
  server), connect a msgpack socket client from the test to the fixed
  default port (19536), `initialize`, call commands, assert
  results — and thus implicitly verify the whole msgpack↔QVariant
  conversion, the async dispatch and the GUI-thread marshalling. The test
  uses the `u-msgpack-python` package for the MessagePack encoding.

## 7. Milestones

1. ~~v1: rust-core JSON-RPC server + cargo tests~~ **done, superseded.**
2. ~~v1: FFI + C++ `RpcServer` skeleton + `api.json` + docs pipeline~~
   **done, superseded.**
3. **Protocol v2 in rust-core** — `rmp`/`rmpv` deps; new `protocol.rs`
   (types 0–3, handshake), `codec.rs` (self-delimiting stream + 32 MiB
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
   covering handshake, ping, `ProjectEditor/list`/`get`, the full
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
- **Access control is user-approval based:** any local process may
  request a connection; the user decides per Ed25519 key (and revoking a
  confirmed key currently requires editing QSettings by hand, see
  below). Combined with the missing server verification (section 9) this
  is not yet safe in multi-user environments — add the instance registry
  (section 9) before shipping.
- **Spawned-process tokens are not implemented:** the token path of
  `authenticate` is currently always rejected, so the interactive
  confirmation is the only working authentication mechanism. Add the
  token mechanism + instance registry (section 9) to make plugins
  (started by the application) work without user interaction.
- **Connection limit:** at most 20 simultaneous connections are accepted
  (`MAX_CONNECTIONS`); additional connections are closed immediately.
  The limit bounds the resource usage of misbehaving clients and is
  arbitrary — raise it if a use case needs more.
- **No authentication timeout:** a connection may stay unauthenticated
  forever, and the C++ nonce map grows with every connection which calls
  `initialize` (bounded by the number of connections, but never shrunk).
- **Unbounded writer queues:** replies/notifications are pushed into
  per-connection queues without backpressure — a client which stops
  reading its socket lets the server-side queue grow (only bounded by
  what the application emits). Add backpressure/drop behavior before
  shipping.
- **No rate limiting:** a local process can pipeline arbitrary amounts
  of `initialize`/`authenticate` requests (e.g. spamming
  connection-request notifications with unknown keys). Revisit together
  with the instance file.
- **No key management UI:** confirmed Ed25519 keys are stored in
  QSettings (`rpc/known_clients`) and can neither be listed nor revoked
  from the application; concurrent confirmations of the same key can
  even create duplicate entries. Add a management UI before the feature
  becomes user-facing.
- **Sandboxing (Flatpak/Snap):** TCP on 127.0.0.1 works because sandboxes
  share the host network namespace — *if* network permissions are granted
  (Flatpak `share=network`; Snap `network` + `network-bind` plugs —
  packaging note). The future instance file must live in
  `QStandardPaths::RuntimeLocation` (the only sandbox-proof location);
  the recorded PID is sandbox-internal and informational only; liveness =
  connect + handshake. For web clients (if WS ever happens): browsers
  allow localhost WS connections from *any* website, making the
  authentication essential — and mixed content only allows it from
  `http://localhost` / `file://` pages.
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

The current design authenticates the *client* to the application, but
not the other way around: A client connecting by port cannot verify that
the peer is really LibrePCB. A malicious local process could squat the
RPC port before the application starts and impersonate the application,
or relay the handshake between a client and the real application (the
user would then approve the client's key while the attacker sits in the
middle). Note that Ed25519 authentication leaves no secret on the client
side to harvest — the attacker can neither steal credentials nor replay
a nonce-based proof — but a relayed connection can be observed and
modified.

To be fixed with: the instance file (see above) carries a random
per-process `secret` (file mode 0600, so other users cannot read it),
and `initialize` is extended to prove the server's knowledge of that
secret with `HMAC-SHA256(secret, nonce)`. Clients disconnect on mismatch.
Since the `initialize` structure is frozen forever, this is added
additively as a *new result member* in a higher protocol version (old
clients ignore unknown keys).

Details (file layout, fallbacks, tests, GUI tool) to be worked out when
implementing. Until then, clients connecting by port are unsafe in
multi-user environments.
