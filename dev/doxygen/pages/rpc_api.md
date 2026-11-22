RPC API Reference {#doc_rpc_api_reference}
==================

[TOC]

This reference documents the commands (data types, error types, classes with their methods and events) which the RPC server offers. It is generated from `dev/rpc/rpc.yml`, the single source of truth of the API. The transport protocol (framing, handshake, conventions) is described in @ref doc_rpc_api.

> Note: This reference is about controlling a *running* LibrePCB application. For the *online* REST API used to fetch libraries, see @ref doc_server_api.

# Data Types {#doc_rpc_api_types}

## ProjectSummary {#doc_rpc_api_type_ProjectSummary}

A summary of an open project.

**Fields:**

| Name | Type | Description |
|------|------|-------------|
| `obj` | handle | The opaque handle of the project (to be passed back in the "obj" key of requests targeting this project). |
| `uuid` | string | The project UUID (null if the project file contains an invalid UUID). |
| `name` | string | The project name. |
| `author` | string | The project author. |
| `version` | string | The project version. |
| `path` | string | The absolute path of the project file. |
| `dirty` | bool | Whether the project has unsaved changes (i.e. its undo stack is not clean). |

---

## ProjectDetails {#doc_rpc_api_type_ProjectDetails}

Details about an open project.

**Fields:**

| Name | Type | Description |
|------|------|-------------|
| `obj` | handle | The opaque handle of the project (to be passed back in the "obj" key of requests targeting this project). |
| `uuid` | string | The project UUID. |
| `name` | string | The project name. |
| `author` | string | The project author. |
| `version` | string | The project version. |
| `path` | string | The absolute path of the project file. |
| `dirty` | bool | Whether the project has unsaved changes (i.e. its undo stack is not clean). |
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
| `InvalidMessage` {#doc_rpc_api_error_InvalidMessage} | 10 | Message decoding failed |
| `MessageTooLarge` {#doc_rpc_api_error_MessageTooLarge} | 11 | Message too large |
| `InvalidRequest` {#doc_rpc_api_error_InvalidRequest} | 20 | Invalid request |
| `Unauthenticated` {#doc_rpc_api_error_Unauthenticated} | 30 | Connection is not authenticated |
| `AccessDenied` {#doc_rpc_api_error_AccessDenied} | 31 | Access denied |
| `MethodNotFound` {#doc_rpc_api_error_MethodNotFound} | 32 | Method not found |
| `MethodNotAvailable` {#doc_rpc_api_error_MethodNotAvailable} | 33 | Method not available |
| `InvalidArgs` {#doc_rpc_api_error_InvalidArgs} | 34 | Invalid method arguments |
| `InvalidHandle` {#doc_rpc_api_error_InvalidHandle} | 35 | Invalid object handle |
| `InternalError` {#doc_rpc_api_error_InternalError} | 38 | Internal error |
| `Cancelled` {#doc_rpc_api_error_Cancelled} | 39 | Request cancelled |
| `CommandFailed` {#doc_rpc_api_error_CommandFailed} | 100 | Command failed |

**`InvalidMessage`:** The received message could not be decoded (invalid MessagePack or a truncated value), so the stream framing is broken and the connection is closed without a response.

**`MessageTooLarge`:** The received message exceeded the maximum size of 32 MiB, so the connection is closed without a response.

**`InvalidRequest`:** The request doesn't conform to the message envelope.

**`Unauthenticated`:** A request other than `initialize` was sent on a connection which is not authenticated yet. The first message of a connection must be an `initialize` request.

**`AccessDenied`:** The provided access token doesn't have the permissions to execute the command. If this error is returned by the 'initialize' method, it means that either the provided token is invalid or has expired, or an interactive connection request was denied by the user.

**`MethodNotFound`:** The method was not found, e.g. because it was added in a newer application version than the running one, or the method has been removed from the application and is no longer available.

**`MethodNotAvailable`:** The method exists, but is currently not available. This may be the case if the application is in a state where the requested functionality does not exist.

**`InvalidArgs`:** The method arguments are invalid, e.g. wrong value types, missing required arguments, invalid values, or unknown enum values.

**`InvalidHandle`:** The provided object handle is invalid or has expired (e.g. object was deleted in the mean time). For example, this error is returned when accessing a project after it has been closed.

**`InternalError`:** An unexpected internal error occurred, e.g. a command handler returned invalid data. This is a bug in LibrePCB, please open an issue on GitHub.

**`Cancelled`:** The request was cancelled by the client and the command stopped its work when it could. Note that if the work already completed, the normal result is sent instead (exactly one response per request).

**`CommandFailed`:** The command execution failed in an unspecified way, i.e. none of the specific error codes fit. The message contains details about the failure.

---

# Commands {#doc_rpc_api_commands}

All commands below are dispatched to the application and executed on its GUI thread. Method names are `ClassName.method_name`. Non-static methods receive their receiver handle in the reserved `obj` key of the params.

## Class Project {#doc_rpc_api_class_Project}

A project opened in the application.

**Handle (obj):** An opaque object handle for the project (as embedded in the responses).

### Project.list {#doc_rpc_api_method_Project_list}

List all open projects.

None.

**Result:** `map`

**Fields:**

| Name | Type | Description |
|------|------|-------------|
| `projects` | array of ProjectSummary | All open projects, in the order of their editor tabs. |

**Since:** 2.2.0

---

### Project.open {#doc_rpc_api_method_Project_open}

Open a project by file path. If the project is already open, nothing is changed and the summary of the already open project is returned.

**Params:**

| Name | Type | Description |
|------|------|-------------|
| `path` | string | The absolute path of the project file (*.lpp or *.lppz). |

**Result:** `ProjectSummary`

**Since:** 2.2.0

---

### Project.get {#doc_rpc_api_method_Project_get}

Get details about an open project.

None.

**Result:** `ProjectDetails`

**Since:** 2.2.0

---

## Class UndoStack {#doc_rpc_api_class_UndoStack}

The undo stack of an open project.

**Handle (obj):** An opaque object handle for the project whose undo stack to use (as embedded in the responses).

### UndoStack.get_status {#doc_rpc_api_method_UndoStack_get_status}

Get the state of the project's undo stack.

None.

**Result:** `UndoStackStatus`

**Since:** 2.2.0

---

### UndoStack.undo {#doc_rpc_api_method_UndoStack_undo}

Undo the last command of the project's undo stack, i.e. the same as pressing Ctrl+Z in the GUI. The command stays on the redo stack, so the user can redo it in the GUI.

None.

**Result:** `UndoStackStatus`

**Since:** 2.2.0

---

### UndoStack.redo {#doc_rpc_api_method_UndoStack_redo}

Redo the last undone command of the project's undo stack, i.e. the same as pressing Ctrl+Shift+Z in the GUI.

None.

**Result:** `UndoStackStatus`

**Since:** 2.2.0

---

## Class WorkspaceDb {#doc_rpc_api_class_WorkspaceDb}

The library database of the currently opened workspace.

### WorkspaceDb.watch {#doc_rpc_api_method_WorkspaceDb_watch}

Watch the library database: While the request is pending, request messages (type 3) about library scans are sent, with the payload `{"progress": <int 0..100>}` (100 means no scan is running). The request stays pending until the client cancels it (message type 4), which stops the watch and finishes the request with the error type `Cancelled`.

None.

**Result:** `null`

**Since:** 2.2.0

