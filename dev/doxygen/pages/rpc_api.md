RPC Server API {#doc_rpc_api}
===============

[TOC]

The RPC server allows external tools (e.g. MCP servers, plugins or scripting libraries) to control a running LibrePCB application. This page documents the transport protocol and all available commands. The commands are generated from `share/librepcb/rpc/api.json`, so the same information is also available at runtime through the `rpc.discover` method.

> Note: This page is about controlling a *running* LibrePCB application. For the *online* REST API used to fetch libraries, see @ref doc_server_api.

# Protocol {#doc_rpc_api_protocol}

The RPC server listens on a local TCP socket (`127.0.0.1`, port assigned by
the operating system) and accepts an unlimited number of connections. The
wire protocol is [JSON-RPC 2.0](https://www.jsonrpc.org/specification), with
exactly one message per line (newline-delimited JSON). This makes the server
easy to debug with tools like `nc 127.0.0.1 <port>`.

## Handshake {#doc_rpc_api_protocol_handshake}

The first message of a connection must be an `initialize` request carrying
the authentication token. Any other first message (or a wrong token) leads
to an error response and an immediate disconnect. Also a previously
authenticated connection is disconnected if a second `initialize` request
contains a wrong token.

~~~
{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"token":"<token>"}}
~~~

The success result contains the protocol version and some application
information:

~~~
{"jsonrpc":"2.0","id":1,"result":{"appVersion":"1.3.0","pid":1234,\
"protocolVersion":"1","workspace":"~/LibrePCB-Workspace"}}
~~~

## Protocol-Level Methods {#doc_rpc_api_protocol_methods}

These methods are handled by the server itself, i.e. they are not
dispatched to the application:

| Method         | Description                                                      |
|----------------|------------------------------------------------------------------|
| `initialize`   | Handshake & authentication (see above).                          |
| `ping`         | Returns an empty result, useful to test a connection.            |
| `rpc.discover` | Returns the OpenRPC API schema (the same document as this page).|

Any other method is dispatched to the application, i.e. it must be listed in
the commands section below. Unknown methods are answered with error
`-32601`.

## Error Codes {#doc_rpc_api_protocol_errors}

| Code   | Meaning                                                  |
|--------|----------------------------------------------------------|
| -32700 | Parse error (the message is not valid JSON).             |
| -32600 | The message is not a valid JSON-RPC 2.0 request.          |
| -32601 | The method does not exist.                               |
| -32602 | Invalid method parameter(s).                             |
| -32603 | Internal error (e.g. the command handler panicked).      |
| -32001 | Not authenticated (invalid or missing token).            |
| -32002 | Command failed (e.g. the command threw an exception).     |
| -32003 | The requested project is not open.                       |
| -32004 | The server is shutting down.                              |

## Request Semantics {#doc_rpc_api_protocol_semantics}

Requests on a single connection are processed strictly sequentially, i.e.
the server does not process the next request of a connection before the
current one was fully dispatched. Within the application, all commands are
executed on the GUI thread, so commands from multiple connections are
automatically serialized. Clients which want to run commands in parallel
need to open multiple connections.

Note that the server is work in progress: The token is currently hardcoded
(not random per instance) and the port is not discoverable yet, see the
notes in `dev/plans/rpc-server.md`.


# Commands {#doc_rpc_api_commands}

All commands below are dispatched to the application and executed on its GUI thread. If not stated otherwise, commands are read-only and do not modify any state. Mutating commands operate on the undo stack, i.e. the user can undo them in the GUI.

## project/list {#doc_rpc_api_project_list}

List all open projects

Returns a list of all projects currently opened in the application. The returned projects are identified by their UUID, which is used as parameter for all project-related commands.

**Params:**

None.

**Result:**

| Key | Type | Description |
|-----|------|-------------|
| `projects` | array |  |

**Errors:**

| Code | Message |
|------|---------|
| -32002 | Command failed |

**Examples:**

**No projects open:**

~~~{.json}
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "project/list",
  "params": []
}
~~~

~~~{.json}
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "projects": []
  }
}
~~~

---

## project/get {#doc_rpc_api_project_get}

Get details about an open project

Returns the metadata, all schematic sheets and all boards of an open project.

**Params:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `project` | string | yes | The UUID of the project (as returned by project/list) |

**Result:**

| Key | Type | Description |
|-----|------|-------------|
| `uuid` | string |  |
| `name` | string |  |
| `author` | string |  |
| `version` | string |  |
| `path` | string |  |
| `modified` | boolean |  |
| `sheets` | array | All schematic sheets of the project |
| `boards` | array | All boards of the project |

**Errors:**

| Code | Message |
|------|---------|
| -32003 | Project not open |

**Examples:**

**Example:**

~~~{.json}
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "project/get",
  "params": {
    "project": "0b1c8866-eae5-4b67-a6b2-5e2b5aeb0f0b"
  }
}
~~~

~~~{.json}
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "uuid": "0b1c8866-eae5-4b67-a6b2-5e2b5aeb0f0b",
    "name": "Example Project",
    "author": "Jane Doe",
    "version": "v1",
    "path": "/home/user/Example Project/Example Project.lpp",
    "modified": false,
    "sheets": [
      {
        "uuid": "3aa64ba1-6c79-414c-8e6a-55a51e6e3f2d",
        "name": "Sheet 1"
      }
    ],
    "boards": [
      {
        "uuid": "c6250d82-3090-4a35-9c47-8121dbd6f384",
        "name": "default"
      }
    ]
  }
}
~~~

---

## undostack/getStatus {#doc_rpc_api_undostack_getStatus}

Get the state of a project's undo stack

Returns whether undo/redo is currently possible for the given project, and the corresponding command texts.

**Params:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `project` | string | yes | The UUID of the project |

**Result:**

| Key | Type | Description |
|-----|------|-------------|
| `canUndo` | boolean | Whether undo is possible |
| `canRedo` | boolean | Whether redo is possible |
| `undoText` | string | The text of the undo command (empty if N/A) |
| `redoText` | string | The text of the redo command (empty if N/A) |
| `clean` | boolean | Whether the stack is in the clean state (the state of the last save) |

**Errors:**

| Code | Message |
|------|---------|
| -32003 | Project not open |

**Examples:**

**Example:**

~~~{.json}
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "undostack/getStatus",
  "params": {
    "project": "0b1c8866-eae5-4b67-a6b2-5e2b5aeb0f0b"
  }
}
~~~

~~~{.json}
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "canUndo": false,
    "canRedo": false,
    "undoText": "",
    "redoText": "",
    "clean": true
  }
}
~~~

---

## undostack/undo {#doc_rpc_api_undostack_undo}

Undo the last command of a project

Undoes the last command in the given project's undo stack, i.e. the same as pressing Ctrl+Z in the GUI. The command appears in the undo stack of the GUI, so it can be redone by the user.

**Params:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `project` | string | yes | The UUID of the project |

**Result:**

The new state of the undo stack

| Key | Type | Description |
|-----|------|-------------|
| `canUndo` | boolean |  |
| `canRedo` | boolean |  |
| `undoText` | string |  |
| `redoText` | string |  |
| `clean` | boolean |  |

**Errors:**

| Code | Message |
|------|---------|
| -32003 | Project not open |
| -32002 | Command failed |

**Examples:**

**Example:**

~~~{.json}
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "undostack/undo",
  "params": {
    "project": "0b1c8866-eae5-4b67-a6b2-5e2b5aeb0f0b"
  }
}
~~~

~~~{.json}
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "canUndo": false,
    "canRedo": true,
    "undoText": "",
    "redoText": "Add component",
    "clean": false
  }
}
~~~

---

## undostack/redo {#doc_rpc_api_undostack_redo}

Redo the last undone command of a project

Redoes the last undone command in the given project's undo stack, i.e. the same as pressing Ctrl+Shift+Z in the GUI.

**Params:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `project` | string | yes | The UUID of the project |

**Result:**

The new state of the undo stack

| Key | Type | Description |
|-----|------|-------------|
| `canUndo` | boolean |  |
| `canRedo` | boolean |  |
| `undoText` | string |  |
| `redoText` | string |  |
| `clean` | boolean |  |

**Errors:**

| Code | Message |
|------|---------|
| -32003 | Project not open |
| -32002 | Command failed |

**Examples:**

**Example:**

~~~{.json}
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "undostack/redo",
  "params": {
    "project": "0b1c8866-eae5-4b67-a6b2-5e2b5aeb0f0b"
  }
}
~~~

~~~{.json}
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "canUndo": true,
    "canRedo": false,
    "undoText": "Add component",
    "redoText": "",
    "clean": false
  }
}
~~~

