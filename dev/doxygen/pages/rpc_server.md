RPC Server API {#doc_rpc_api}
===============

[TOC]

The RPC server allows external tools (e.g. MCP servers, plugins or
scripting libraries) to control a running LibrePCB application. This page
describes the transport protocol of the server; the available commands
(data types, error types, classes with their methods) are documented in
\subpage doc_rpc_api_reference.

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

The first message of a connection must be an `initialize` request. Any
other first message is answered with an error response (without being
dispatched) and the connection stays open, but stays unauthenticated.
There are two ways to authenticate:

- **With a token:** The params carry a `token` string. If the token was
  issued by this application instance (see below) and is still valid, the
  connection is authenticated. Otherwise the request is answered with an
  error (code 25) and the connection stays open, but stays
  unauthenticated.
- **Interactively:** The params carry a `name` string identifying the
  client. The application asks the user whether to accept the connection
  (via a notification). If the user accepts it, the connection is
  authenticated and a new token is issued, which the client shall store
  for future connections. If the user denies the request (or the client
  cancels it), the request is answered with an error (code 25).

The success result is a map with some application information:

~~~{.msgpack}
[1, 1, nil, {"format": "1.0", "server": "librepcb", "session":
"783ea436-b2bc-4a1e-95a1-d5600c4a26d6", "token":
"0f0e7d09-457c-4924-a21f-d8d5341f8d55", "version": "2.1.2",
"workspace": "~/LibrePCB-Workspace"}]
~~~

The fields have the following meaning:

- `format`: The RPC interface format version (currently `"1.0"`).
  Clients must verify the major version and reject other major versions;
  a higher minor version means additive, backward-compatible changes.
- `server`: Always `"librepcb"` (so clients can verify the server).
- `version`: The application version.
- `session`: A UUID identifying the application *process*: It changes
  whenever the application is restarted, so clients can detect that.
- `token`: The access token for this connection — either the one provided
  in the request, or a freshly issued one (see above).
- `workspace`: The path of the currently opened workspace.

Clients use `version` together with the per-method `since`
from the specification file to determine which methods the server
supports: Methods added after the revision of the client's specification
are unknown to the client (calling them results in error code 22,
`MethodNotFound`).

Protocol-Level Methods {#doc_rpc_api_protocol_methods}
----------------------

These methods are handled by the application itself, i.e. they are not
part of the class-based command API:

| Method       | Description                                    |
|--------------|------------------------------------------------|
| `initialize` | Handshake & authentication (see above).        |
| `ping`       | Returns nil, useful to test a client.           |

Messages {#doc_rpc_api_messages}
--------

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
  answered with error code 20 (`InvalidRequest`) and the connection stays
  open. However, values outside the protocol's value domain (e.g. `ext`
  values) result in error code 23 (`InvalidParams`).

Params & Conventions {#doc_rpc_api_params}
--------------------

The `params` of a request are a map (or nil, which means an empty map):
The arguments are passed as named fields, plus the reserved key `"obj"`
which carries the receiver handle for non-static methods (an argument may
never be named `obj`).

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

Values are from the restricted MessagePack domain: null, bool, int (i64
or u64),
float (f64), string, binary, array and map (with string keys). Enums are
transferred as strings (see the specification file for the allowed
values). Numeric conventions: Lengths in nanometers, angles in
microdegrees.

Error Handling {#doc_rpc_api_error_handling}
--------------

The `error` element of a response is a map with the keys `code` and
`message`, plus optionally additional properties. The error codes are
grouped: 10–19 transport errors, 20–29 protocol errors, 30–39 command
errors, 100– command execution errors. All codes are listed in the
\ref doc_rpc_api_errors "error types" section of the reference.

- Transport errors (10–19) are never *sent*: When one of them occurs, the
  stream framing is broken and the connection is closed without a
  response.
- Protocol errors (20–29) keep the connection open.
- Command errors (30–39) and command execution errors (100–) are specific
  to the executed command. The canonical message of an error code is
  sent on the wire, optionally followed by an application-specific
  detail message (separated by a colon).

The specification file is the authoritative list of error types with
their codes, messages and additional properties.
