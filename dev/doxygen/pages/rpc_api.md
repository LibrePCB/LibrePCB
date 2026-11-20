RPC Server API {#doc_rpc_api}
===============

[TOC]

The RPC server allows external tools (e.g. MCP servers, plugins or scripting libraries) to control a running LibrePCB application. This page documents the transport protocol and all available commands. It is generated from `share/librepcb/rpc/rpc.yaml`, the single source of truth of the API.

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
[1, 1, nil, {"appVersion": "2.1.2", "pid": 1234,\
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



# Data Types {#doc_rpc_api_types}

## ProjectSummary {#doc_rpc_api_type_ProjectSummary}

A summary of an open project.

**Fields:**

| Name | Type | Description |
|------|------|-------------|
| `uuid` | string | The project UUID (null if the project file contains an invalid UUID). |
| `name` | string | The project name. |
| `author` | string | The project author. |
| `version` | string | The project version. |
| `path` | string | The absolute path of the project file. |
| `modified` | bool | Whether the project was modified since the last save. |

---

## ProjectDetails {#doc_rpc_api_type_ProjectDetails}

Details about an open project.

**Fields:**

| Name | Type | Description |
|------|------|-------------|
| `uuid` | string | The project UUID. |
| `name` | string | The project name. |
| `author` | string | The project author. |
| `version` | string | The project version. |
| `path` | string | The absolute path of the project file. |
| `modified` | bool | Whether the project was modified since the last save. |
| `sheets` | array of SchematicSummary | All schematic sheets of the project. |
| `boards` | array of BoardSummary | All boards of the project. |

---

## SchematicSummary {#doc_rpc_api_type_SchematicSummary}

A summary of a schematic sheet.

**Fields:**

| Name | Type | Description |
|------|------|-------------|
| `uuid` | string | The schematic UUID. |
| `name` | string | The schematic name. |

---

## BoardSummary {#doc_rpc_api_type_BoardSummary}

A summary of a board.

**Fields:**

| Name | Type | Description |
|------|------|-------------|
| `uuid` | string | The board UUID. |
| `name` | string | The board name. |

---

## UndoStackStatus {#doc_rpc_api_type_UndoStackStatus}

The state of a project's undo stack.

**Fields:**

| Name | Type | Description |
|------|------|-------------|
| `canUndo` | bool | Whether undo is currently possible. |
| `canRedo` | bool | Whether redo is currently possible. |
| `undoText` | string | The text of the undo command (empty if N/A). |
| `redoText` | string | The text of the redo command (empty if N/A). |
| `clean` | bool | Whether the stack is in the clean state (the state of the last save). |

---

# Error Types {#doc_rpc_api_errors}

The `error` element of a response is a map with the keys `code` and `message` (and possibly additional properties, listed below):

| Error | Code | Message |
|-------|------|---------|
| `CommandFailed` {#doc_rpc_api_error_CommandFailed} | -32002 | Command failed |
| `ProjectNotOpen` {#doc_rpc_api_error_ProjectNotOpen} | -32003 | Project not open |

**`CommandFailed`:** The command could not be executed, e.g. because it threw an exception. The message contains details about the failure.

**`ProjectNotOpen`:** The requested project is not opened in the application.

**Properties:**

| Name | Type | Description |
|------|------|-------------|
| `project` | string | The requested project handle (UUID). |

---

# Commands {#doc_rpc_api_commands}

All commands below are dispatched to the application and executed on its GUI thread. Method names are `ClassName.method_name`. Non-static methods receive their receiver handle in the reserved `obj` key of the params.

## Class Project {#doc_rpc_api_class_Project}

A project opened in the application.

**Handle (obj):** The UUID of the open project (as returned by Project.list).

### Project.list {#doc_rpc_api_method_Project_list}

List all open projects.

None.

**Result:** `map`

**Fields:**

| Name | Type | Description |
|------|------|-------------|
| `projects` | array of ProjectSummary | All open projects, in the order of their editor tabs. |

**Errors:**

None (besides protocol-level errors).

**Added in version:** 2.1.2

---

### Project.get {#doc_rpc_api_method_Project_get}

Get details about an open project.

None.

**Result:** `ProjectDetails`

**Errors:**

| Error | Code | Message |
|-------|------|---------|
| `ProjectNotOpen` | -32003 | Project not open |

**Added in version:** 2.1.2

---

## Class UndoStack {#doc_rpc_api_class_UndoStack}

The undo stack of an open project.

**Handle (obj):** The UUID of the project whose undo stack to use.

### UndoStack.get_status {#doc_rpc_api_method_UndoStack_get_status}

Get the state of the project's undo stack.

None.

**Result:** `UndoStackStatus`

**Errors:**

| Error | Code | Message |
|-------|------|---------|
| `ProjectNotOpen` | -32003 | Project not open |

**Added in version:** 2.1.2

---

### UndoStack.undo {#doc_rpc_api_method_UndoStack_undo}

Undo the last command of the project's undo stack, i.e. the same as pressing Ctrl+Z in the GUI. The command stays on the redo stack, so the user can redo it in the GUI.

None.

**Result:** `UndoStackStatus`

**Errors:**

| Error | Code | Message |
|-------|------|---------|
| `ProjectNotOpen` | -32003 | Project not open |
| `CommandFailed` | -32002 | Command failed |

**Added in version:** 2.1.2

---

### UndoStack.redo {#doc_rpc_api_method_UndoStack_redo}

Redo the last undone command of the project's undo stack, i.e. the same as pressing Ctrl+Shift+Z in the GUI.

None.

**Result:** `UndoStackStatus`

**Errors:**

| Error | Code | Message |
|-------|------|---------|
| `ProjectNotOpen` | -32003 | Project not open |
| `CommandFailed` | -32002 | Command failed |

**Added in version:** 2.1.2

