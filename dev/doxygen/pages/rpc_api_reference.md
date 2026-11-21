RPC API Reference {#doc_rpc_api_reference}
==================

[TOC]

This reference documents the commands (data types, error types, classes with their methods and events) which the RPC server offers. It is generated from `share/librepcb/rpc/rpc.yaml`, the single source of truth of the API. The transport protocol (framing, handshake, conventions) is described in @ref doc_rpc_api.

> Note: This reference is about controlling a *running* LibrePCB application. For the *online* REST API used to fetch libraries, see @ref doc_server_api.

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
| `MessageDecodingFailed` {#doc_rpc_api_error_MessageDecodingFailed} | 10 | Message decoding failed |
| `MessageTooLarge` {#doc_rpc_api_error_MessageTooLarge} | 11 | Message too large |
| `InvalidRequest` {#doc_rpc_api_error_InvalidRequest} | 20 | Invalid Request |
| `Unauthenticated` {#doc_rpc_api_error_Unauthenticated} | 21 | Invalid or missing token |
| `MethodNotFound` {#doc_rpc_api_error_MethodNotFound} | 22 | Method not found |
| `InvalidParams` {#doc_rpc_api_error_InvalidParams} | 23 | Invalid params |
| `InternalError` {#doc_rpc_api_error_InternalError} | 24 | Internal error |
| `CommandFailed` {#doc_rpc_api_error_CommandFailed} | 100 | Command failed |
| `ProjectNotOpen` {#doc_rpc_api_error_ProjectNotOpen} | 101 | Project not open |
| `Cancelled` {#doc_rpc_api_error_Cancelled} | 102 | Request cancelled |

**`MessageDecodingFailed`:** The received message could not be decoded (invalid MessagePack or a truncated value), so the stream framing is broken and the connection is closed without a response.

**`MessageTooLarge`:** The received message exceeded the maximum size of 32 MiB, so the connection is closed without a response.

**`InvalidRequest`:** The request doesn't conform to the message envelope.

**`Unauthenticated`:** The connection is not authenticated. The first message of a connection must be an `initialize` request carrying the correct token.

**`MethodNotFound`:** The method does not exist or is not available, e.g. because it was added in a newer application version than the running one.

**`InvalidParams`:** The request parameters are invalid, e.g. values outside the protocol's value domain or a missing/invalid receiver handle.

**`InternalError`:** An unexpected internal error occurred, e.g. a command handler panicked or returned invalid data.

**`CommandFailed`:** The command could not be executed, e.g. because it threw an exception. The message contains details about the failure.

**`ProjectNotOpen`:** The requested project is not opened in the application.

**Properties:**

| Name | Type | Description |
|------|------|-------------|
| `project` | string | The requested project handle (UUID). |

**`Cancelled`:** The request was cancelled by the client and the command stopped its work when it could. Note that if the work already completed, the normal result is sent instead (exactly one response per request).

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
| `ProjectNotOpen` | 101 | Project not open |

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
| `ProjectNotOpen` | 101 | Project not open |

**Added in version:** 2.1.2

---

### UndoStack.undo {#doc_rpc_api_method_UndoStack_undo}

Undo the last command of the project's undo stack, i.e. the same as pressing Ctrl+Z in the GUI. The command stays on the redo stack, so the user can redo it in the GUI.

None.

**Result:** `UndoStackStatus`

**Errors:**

| Error | Code | Message |
|-------|------|---------|
| `ProjectNotOpen` | 101 | Project not open |
| `CommandFailed` | 100 | Command failed |

**Added in version:** 2.1.2

---

### UndoStack.redo {#doc_rpc_api_method_UndoStack_redo}

Redo the last undone command of the project's undo stack, i.e. the same as pressing Ctrl+Shift+Z in the GUI.

None.

**Result:** `UndoStackStatus`

**Errors:**

| Error | Code | Message |
|-------|------|---------|
| `ProjectNotOpen` | 101 | Project not open |
| `CommandFailed` | 100 | Command failed |

**Added in version:** 2.1.2

