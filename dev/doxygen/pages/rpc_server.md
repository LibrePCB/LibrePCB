RPC Server API {#doc_rpc_api}
===============

[TOC]

The RPC server allows external tools (e.g. MCP servers, plugins or
scripting libraries) to control a running LibrePCB application. This page
describes the transport protocol of the server; the available commands
(data types, error types, classes with their methods) are documented in
\subpage doc_rpc_api_reference. Guidelines for **developers** of the RPC
interface (payload conventions, object handle handling, ...) are
described in \subpage doc_rpc_guidelines.

The reference is generated from `dev/rpc/rpc.yml`, the single
source of truth of the API.

> Note: This page is about controlling a *running* LibrePCB application.
> For the *online* REST API used to fetch libraries, see @ref
> doc_server_api.

Transport & Framing {#doc_rpc_api_transport}
-------------------

The RPC server listens on a local TCP socket (`127.0.0.1`) on a fixed
default port: 19536, which is the letters "LP" (for LibrePCB)
interpreted as a hexadecimal number (0x4C50; unassigned in the IANA
registry and below the ephemeral port ranges of all major platforms). If
the port is already in use (e.g. by another LibrePCB instance), the next
higher port is used.
The wire format is [MessagePack](https://msgpack.org), decoded directly from
the stream (self-delimiting, like the official MessagePack-RPC protocol,
no length prefix). A decoder which blocks mid-value until more bytes
arrive never re-parses partial data. The size of a single message is
limited to 32 MiB (enforced mid-parse). If the stream contains invalid
MessagePack, the framing is broken and the connection is closed without a
response.

Handshake {#doc_rpc_api_handshake}
---------

The first message of a connection must be an
[`initialize`](@ref doc_rpc_api_method_initialize) request, followed by an
[`authenticate`](@ref doc_rpc_api_method_authenticate) request. Any other
message is answered with an error response (without being dispatched) and
the connection stays open, but stays unauthenticated.

**Version negotiation:** The client passes the highest format version it
supports in the `format` argument (`"MAJOR.MINOR"`). The server replies
with the *negotiated version*, which the whole connection then speaks:

> 1. Versions are `"MAJOR.MINOR"`. Within a major, higher versions are
>    backwards compatible (strict supersets). Majors may drop features.
> 2. `initialize` is the mandatory first message and its structure never
>    changes. A missing or unparsable `format` argument is answered with
>    [error code 40](@ref doc_rpc_api_errors).
> 3. The negotiated version is `min(client_format, server_format)`. The
>    server supports every version between its minimum and its
>    `server_format`; if the client's offer is below that minimum, the
>    server responds with its minimum. The server never rejects on version
>    grounds — a client which cannot speak the negotiated version simply
>    closes the connection.
> 4. All messages on the connection conform to the negotiated version, in
>    both directions. A higher minor version therefore means additive,
>    backward-compatible changes *relative to the negotiated version*: A
>    server never emits anything the client's offered version does not
>    cover.

The `initialize` response carries a fresh `nonce` for authenticating.
Host verification is deliberately not part of the protocol yet — clients
transmit nothing reusable, so a wrong server gains nothing but fabricated
responses; a later version may add it (unknown result keys are ignored by
old clients, so the frozen handshake stays intact).

**Authentication** is a separate request, so the handshake stays minimal
and frozen while the authentication flow may evolve:

- **External clients** pass their Ed25519 `pubkey` (64 hex characters) and
  a `proof` — the signature (128 hex characters) over the `nonce`. If the
  user has confirmed this key before, the connection is authenticated
  silently. If the key is unknown, the application asks the user whether
  to accept the client (via a notification): On acceptance the key is
  remembered and the connection is authenticated; on denial the request is
  answered with [error code 33](@ref doc_rpc_api_errors); if the client
  cancels it, with [error code 99](@ref doc_rpc_api_errors). The
  application stores only *public* keys — no secrets are stored or
  transmitted, and a signature over a per-connection nonce cannot be
  replayed.
- **Spawned processes** (e.g. plugins started by the application itself)
  simply pass the `token` they received out-of-band (e.g. by an
  environment variable). The application issued that token itself, so no
  user interaction is needed. The token is only valid for the lifetime of
  the issuing application instance, and this path requires no cryptography
  at all in the client. *Not implemented yet: No tokens are issued, so
  this argument is currently always rejected.*

Connections stay open on all errors; the client closes them.

The `authenticate` success result is a map with application information:

~~~{.msgpack}
[1, 2, 0, {"server": "librepcb", "session":
"783ea436-b2bc-4a1e-95a1-d5600c4a26d6", "version": "2.1.2"}]
~~~

- `server`: Always `"librepcb"` (so clients can verify the server).
- `version`: The application version.
- `session`: A UUID identifying the application *process*: It changes
  whenever the application is restarted, so clients can detect that.

**Endpoint discovery** *(planned, not implemented yet)*: At startup the
server shall write `{pid, port, session}` to a file with 0600 permissions
in `$XDG_RUNTIME_DIR` (the `DISPLAY`/`SSH_AUTH_SOCK` pattern). Clients
shall connect to the port from that file — a process squatting the
default port then never receives a connection — and may verify the
`session` against the file, which confirms they reached exactly the
process which wrote it. The file is removed when the server stops.

Clients use `version` together with the per-method `since`
from the specification file to determine which methods the server
supports: Methods added after the revision of the client's specification
are unknown to the client (calling them results in
[error code 31](@ref doc_rpc_api_errors)).

Messages {#doc_rpc_api_messages}
--------

The protocol is *bidirectional*: Both sides of a connection can initiate
method calls in exactly the same way, using the same message types (this
is what will allow e.g. the application to call methods provided by
plugins). Authentication (see above) applies to client-initiated
requests only.

All messages are MessagePack arrays whose first element is the message
type:

| Type | Message      | Direction     | Shape                         |
|------|--------------|---------------|-------------------------------|
| 0    | Request      | both          | `[0, msgid, method, params]`  |
| 1    | Response     | callee→caller | `[1, msgid, status, payload]` |
| 2    | Notification | callee→caller | `[2, msgid, payload]`         |
| 3    | Cancel       | caller→callee | `[3, msgid]`                  |

The `status` of a response is `0` on success (the payload carries the
result, which may be nil) or the error code on failure (the payload is
then a map carrying the error `message`, possibly with additional
properties; see @ref doc_rpc_api_errors).

- `msgid` is an unsigned 32-bit integer, unique per direction while
  pending (a response always travels in the opposite direction of its
  request, so msgids of requests pending in both directions never
  collide). Responses may be sent in any order (pipelining): Requests
  are processed while previous ones are still being processed.
- Exactly one response is sent per request, enforced by the callee. A
  second response (or a response for an unknown or closed connection) is
  silently dropped.
- A notification (type 2, see @ref doc_rpc_api_notifications) is only
  delivered while the request is pending, i.e. before its response.
- With `[3, msgid]` the caller may cancel a pending request.
  Cancellation is cooperative: The callee stops its work when it can and
  responds with a "cancelled" error; if the work already completed, the
  normal result is sent. Either way, exactly one final response arrives.
- Unknown message types are ignored for forward compatibility. A request
  which doesn't conform to the envelope (e.g. a non-string method) is
  answered with an error response ([error code 20]
  (@ref doc_rpc_api_errors)), echoed to the request's msgid — or to the
  reserved msgid 0 if no valid msgid could be determined. The connection
  stays open. However, values outside the protocol's value domain result
  in [error code 40](@ref doc_rpc_api_errors).
- The msgid 0 is reserved for future notifications, i.e. method calls the
  sender is not interested in a response for (which MUST NOT be replied
  to). Senders must not use msgid 0 yet, and it is currently rejected as
  an invalid request.

Notifications {#doc_rpc_api_notifications}
-------------

While a request is pending, the callee may send notifications (type 2)
to the caller, e.g. to report progress or to stream (partial) result
data. Notifications are only delivered before the response — once the
response (or an error) is sent, no further notifications arrive.

The payload is a map built from the following members (all optional):

| Member     | Type | Description |
|------------|------|-------------|
| `progress` | int (0–100) or float (0.0–100.0) | Progress in percent. It may **decrease** when the callee discovers more work — it is never a completion signal. |
| `step`     | int | The current step of a fixed-step operation, 1-based (e.g. "copying the 3rd of 10 files"). |
| `steps`    | int | The total number of steps (may grow while the operation runs). |
| `message`  | string | An optional human-readable message line (e.g. "Copying can2usb.lpp…"). |
| `result`   | map | Partial result data, see below. |

The members `progress`, `step`, `steps` and `message` are *state*: The
latest value replaces the previous one. The `result` member carries
*result data*: Its layout shall be the same as the documented result of
the method, with any subset of members present (data which is not known
yet is simply omitted).

Result data is delivered incrementally: The `result` member of every
notification — as well as the response payload, which is not wrapped in
a `result` member — is merged into a *rolling result*, using JSON Merge
Patch semantics as defined in
[RFC 7396](https://datatracker.ietf.org/doc/html/rfc7396), with these
differences:

- Arrays are *appended*: A fragment only carries the items which were
  not delivered yet.
- A `nil` value deletes the corresponding member from the rolling
  result; a nil fragment (or a nil response payload) merges nothing.

The response therefore only carries data which has not been delivered by
notifications yet (a fully streamed response carries nil) — data is
never transferred twice.

~~~{.msgpack}
# ── Generic shape ──────────────────────────────────────────────
[2, <msgid>, {
    "progress": 42,            # int: percent
    # "progress": 42.5,        # …or float — handled the same way
    "step": 3,                 # 1-based: processing the 3rd of 10 steps
    "steps": 10,               # may grow while the operation runs
    "message": "Copying can2usb.lpp…",
    "result": { … },           # partial result data (see above)
}]

# ── Example: streaming parts of the result ─────────────────────
[2, 7, { "progress": 10, "status": "Fetching libraries…" }]
[2, 7, { "result": { "libraries": [ {…lib 1…} ] } }]
[2, 7, { "result": { "libraries": [ {…lib 2…} ] } }]
[2, 7, { "result": { "count": 5 } }]     # scalars may be refined later
[2, 7, { "result": { "count": 9 } }]

# ── Completion: the response carries only the remainder ────────
[1, 7, 0, { "libraries": [ {…lib 3…} ] }]
# → rolling result = {"libraries": [{…1…}, {…2…}, {…3…}], "count": 9}

# ── Fully streamed response (nothing left to deliver) ──────────
[1, 7, 0, nil]
~~~

Params & Results {#doc_rpc_api_params}
----------------

The `params` of a request are a map (or nil, which means an empty map):
The arguments are passed as named fields. Non-static methods
additionally receive their receiver handle in the reserved key `"obj"`
(an argument may never be named `obj`).

The result of a command is either nil (no result) or a map: The fields
are documented per command in the reference. Structs are transferred as
such maps (with the documented fields), so they never need a separate
representation.

Object Handles {#doc_rpc_api_handles}
--------------

Object handles are **opaque values**: Clients obtain them from responses
(embedded under the reserved key `"obj"` in maps which represent objects)
and must pass them back unchanged where a receiver handle is required.
Clients must not make any assumptions about the type or value of handles
— the server may use any MessagePack type to represent an object, and the
representation may change from command to command and from app version to
app version. In the specification file, handles have the special type
`handle`.

A handle is valid as long as the referenced object exists and the RPC
server keeps running: It survives renames, moves and UUID changes of the
object, but it becomes invalid when the object is deleted, and a server
restart invalidates all handles (re-discover objects after
reconnecting). Handle values are never reused.
