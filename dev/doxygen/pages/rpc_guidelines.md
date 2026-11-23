RPC Interface Guidelines {#doc_rpc_guidelines}
========================

[TOC]

This page documents the conventions which the RPC interface follows, as a
guide for **developers** of the interface (i.e. everyone implementing or
designing RPC commands in LibrePCB). **Users** of the interface should
consult \ref doc_rpc_api (protocol) and \ref doc_rpc_api_reference
(reference) instead.

Adding a Command
----------------

New commands are added in four steps:

1. **Declare the method in the specification** (`dev/rpc/rpc.yml`): add
   it to its class, with description, params, result and `since`. Results
   are always a map (declared as field list or struct name) or nil.
2. **Run the generator** (`uv run gen.py` in `dev/rpc/`): it regenerates
   the C++ command headers (`rpc/api/{classname}rpc.h`), the dispatcher
   (`rpc/api/commands.cpp`), the error classes (`rpc/api/rpcerrors.h`) and
   this documentation. Commit the regenerated files.
3. **Implement the method** in the hand-written
   `rpc/api/{classname}rpc.cpp`: a static method with the signature
   `(GuiApplication& app, RpcObjectRegistry& registry, RpcRequest&
   request)`. No registration is needed — the generated dispatcher
   already routes to it.
4. **Reply exactly once**: by returning normally after
   `request.sendSuccess()`, or by throwing an error (see below).

Only the files listed in this page's sections are hand-written;
everything else is generated. New *classes* additionally get an empty
implementation file (created by the generator if it does not exist yet)
and are registered in `libs/librepcb/editor/CMakeLists.txt`
automatically.

Handler Implementation
----------------------

- Handlers run on the **GUI thread** and must **never block**. Long-running
  work is done asynchronously: keep the request alive (it is a
  `std::shared_ptr`, so capture a copy), schedule the work (Qt signals,
  timers, `QtConcurrent`), and reply when it finishes. The Rust server
  guarantees the reply contract, but the handler must make sure the reply
  happens eventually — otherwise the client runs into a timeout.
- **Errors are thrown**: a handler just throws one of the error classes
  (see below); the server catches it and sends the response. No
  `sendError()` calls and no early returns are needed in the happy path.
- **Cancellation is cooperative**: the client may cancel a pending
  request (which emits the `cancelled()` Qt signal and sets the
  `isCancelled()` flag). Long-running handlers should poll the flag or
  connect to the signal, stop their work early — and still reply exactly
  once (typically with the `Cancelled` error, or with the normal result
  if the work already completed).
- **Request-scoped messages** (e.g. progress) can be sent any number of
  times while the request is pending, via `request.sendProgress()`. They
  are dropped automatically once the request is replied.
- When capturing the request *and* an application object for asynchronous
  work, prefer a `QPointer` for the application object: the user may close
  it while the work is running.

Error Handling
--------------

- Every error code of the API has a generated exception class
  (`RpcMethodNotFound`, `RpcInvalidHandle`, `RpcCommandFailed`, ... in
  `rpc/api/rpcerrors.h`), all deriving from `RpcError`. Command
  implementations just *throw* them.
- The message sent on the wire is composed by the Rust server: the
  canonical message of the error code (from the specification), optionally
  followed by the application-provided detail. `what()` of an exception
  only returns the detail.
- The single catch ladder lives in `RpcServer::handleRequest`; command
  implementations never catch RPC errors themselves — *except* in
  asynchronous continuations: there is no framework catch on the GUI
  event loop, so an async continuation must catch `RpcError` itself and
  call `request.sendError(e)`.

Object Handles
--------------

Object handles are identity-based (see \ref doc_rpc_api_params): they
refer to registered objects and are invalid once the object is destroyed.

- Command implementations obtain handles from the registry (passed as the
  `registry` parameter): `registry.registerObject(obj)` embeds a handle
  into a response (registering the object on first use; the same object
  always yields the same handle).
- Lookup is typed and throwing:
  `registry.getAs<ProjectEditor>(request.getArg("obj"))` returns a
  `ProjectEditor&` and throws `RpcInvalidHandle` for unknown handles or
  wrong types. There are no helper wrappers — handlers call it directly.
- Handle values are never reused, not even across server restarts; all
  handles become invalid when the server is stopped.

Value Domain
------------

The params and results of commands are maps of values from the restricted
MessagePack domain (see \ref doc_rpc_api_params). They cross the FFI
boundary as `QVariant` trees:

| msgpack | QVariant |
|---|---|
| nil | invalid QVariant |
| bool | `bool` |
| int | `qlonglong` (large u64 values as `qulonglong`) |
| float | `double` (f32 widens exactly) |
| str | `QString` |
| bin | `QByteArray` |
| array | `QVariantList` |
| map | `QVariantMap` (string keys only) |

The conversion is direct (no intermediate value type): the Rust FFI walks
the msgpack value and builds the QVariant tree via the `qvariant_*` FFI
helpers, and vice versa. Everything else (e.g. `QDateTime`, `ext` types,
non-string map keys) is rejected. Numeric conventions: lengths in
nanometers, angles in microdegrees (see \ref doc_rpc_api_params).

Payload Conventions
-------------------

The commands follow one uniform read pattern, which applies identically to
every object class (projects, schematics, boards, ...):

- Every object class exposes exactly one **`get` member method** returning
  the *details*: its summary plus everything else which is **cheaply
  computable** about the object itself (child summaries, state), so that
  one call yields a complete picture for everyday use. The payload is
  bounded by the object's own size — never the whole project graph.
- **Cheap** means: values which are already held in memory, no scans, no
  blocking work, no data which needs to be computed. Anything not cheap is
  a separate command, never a payload field.
- **Summaries** (returned by `list` methods and embedded in a parent's
  `get` result) carry the minimum to *identify, display and decide*:
  the object handle, name, path and dirty-style state flags. They never
  contain nested collections.

Enumeration Conventions
-----------------------

Collections of objects have exactly **one enumeration point**, chosen by
their owner:

- The **root collection** (currently: the open projects) is enumerated by
  a **static `list` method** on its class, since no parent exists to ask.
- **Small, bounded child collections** (currently: the schematics and
  boards of a project) are enumerated by their **parent's `get` result**,
  which embeds child summaries — including the child handles.
- **Large or volatile child collections** get their own **`list` member
  method** on the child class (with the parent's handle in `obj`).
- Collections which may grow without bound are **paginated** with opaque
  cursors (`cursor`/`nextCursor`, following the Model Context Protocol
  semantics), once the first such collection appears.

Non-Goals
---------

The following are deliberately *not* part of the interface; do not add
them speculatively:

- **Numeric detail levels** (e.g. a `details` argument with 0 = minimum,
  1 = summary, ...). Depth is modeled with **more object classes**
  (e.g. `Schematic.get`), not with payload variants.
- **Field projection** (`fields` arguments) and **expansion** arguments
  (`include`). The protocol is local, so payload sizes are irrelevant;
  the summaries keep responses small where it matters.
- **Pagination before it is needed** (see above).

@see doc_rpc_api @see doc_rpc_api_reference
