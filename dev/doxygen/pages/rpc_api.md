RPC API Reference {#doc_rpc_api_reference}
==================

[TOC]

This reference documents the commands (data types, error types, classes with their methods and events) which the RPC server offers. It is generated from `dev/rpc/rpc.yml`, the single source of truth of the API. The transport protocol (framing, handshake, conventions) is described in @ref doc_rpc_api.

> Note: This reference is about controlling a *running* LibrePCB application. For the *online* REST API used to fetch libraries, see @ref doc_server_api.

# Data Types {#doc_rpc_api_types}

## ProjectEditorSummary {#doc_rpc_api_type_ProjectEditorSummary}

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
| `dirty` | bool | Whether the project has unsaved changes (i.e. its undo stack is not clean, or any manual, untracked changes have been made). |

---

## ProjectEditorDetails {#doc_rpc_api_type_ProjectEditorDetails}

Details about an open project.

**Fields:**

| Name | Type | Description |
|------|------|-------------|
| `uuid` | uuid | The project UUID. |
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
| `uuid` | uuid | The schematic UUID. |
| `name` | string | The schematic name. |

---

## BoardSummary {#doc_rpc_api_type_BoardSummary}

A summary of a board.

**Fields:**

| Name | Type | Description |
|------|------|-------------|
| `uuid` | uuid | The board UUID. |
| `name` | string | The board name. |

---

## UndoStackDetails {#doc_rpc_api_type_UndoStackDetails}

The state of a document's undo stack.

**Fields:**

| Name | Type | Description |
|------|------|-------------|
| `id` | any | An opaque identifier which uniquely identifies the current state. This can be used for example to detect if there were any changes made between two different points in time. |
| `clean` | bool | Whether the stack is in the clean state (the state of the last save). |
| `prev` | string | nil | The text of the previously executed command, i.e. the command which will be reverted by `undo()`. If there is no command to undo, this is `nil`. |
| `next` | string | nil | The text of the previously undone command, i.e. the command which will be executed by `redo()`. If there is no command to redo, this is `nil`. |

---

## WorkspaceDbDetails {#doc_rpc_api_type_WorkspaceDbDetails}

Information about the workspace library database.


---

## BackupRestoreMode (Enum) {#doc_rpc_api_type_BackupRestoreMode}
**Default value:** `abort`

| Value | Description |
|-------|-------------|
| `abort` | Do not attempt to open a file which has an auto-save backup. Abort with an error instead. |
| `discard` | Discard the backup, restore and load the last saved state. |
| `restore` | Restore the auto-save backup, if available. |

---
# Extension Types {#doc_rpc_api_ext_types}

MessagePack extension types: Tagged raw bytes which are opaque to the transport layer (only the ext code and the raw payload cross the wire). Their interpretation is up to the client/server.

## uuid (ext code 0x02) {#doc_rpc_api_ext_type_uuid}

A UUID (RFC 4122), encoded as its 16 bytes in field order (network byte order). This is equivalent to the string representation (type "uuid_str"), but requires only half of the size on the wire.

**Ext code:** `0x02`

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

## Class ProjectEditor {#doc_rpc_api_class_ProjectEditor}

A project opened in the application.

### ProjectEditor.list {#doc_rpc_api_method_ProjectEditor_list}

List all open projects.

None.

**Result:** `map`

**Fields:**

| Name | Type | Description |
|------|------|-------------|
| `projects` | array of ProjectEditorSummary | All open projects, in the order of their editor tabs. |

**Since:** 2.2.0

---

### ProjectEditor.open {#doc_rpc_api_method_ProjectEditor_open}

Open a project by file path. If the project is already open, nothing is changed and the summary of the already open project is returned.

**Params:**

| Name | Type | Description |
|------|------|-------------|
| `path` | string | The absolute path of the project file (*.lpp or *.lppz). |
| `backup` | BackupRestoreMode | If the project directory contains an auto-save backup (i.e. the application crashed while the project was open and had unsaved modifications), this flag specifies how to proceed in that case. |

**Result:** `ProjectEditorSummary`

**Since:** 2.2.0

---

### ProjectEditor.get {#doc_rpc_api_method_ProjectEditor_get}

Get details about an open project.

None.

**Result:** `ProjectEditorDetails`

**Since:** 2.2.0

---

### ProjectEditor.save {#doc_rpc_api_method_ProjectEditor_save}

Save project to disk.

None.

**Result:** `null`

**Since:** 2.2.0

---

### ProjectEditor.close {#doc_rpc_api_method_ProjectEditor_close}

Close project. Attention: If there were unsaved changes in the project, they will be discarded. Call `save` first, if desired.

None.

**Result:** `null`

**Since:** 2.2.0

---

## Class UndoStack {#doc_rpc_api_class_UndoStack}

The undo stack of an open document. The handle may point to a project or a library element.

### UndoStack.get {#doc_rpc_api_method_UndoStack_get}

Get the state of the undo stack.

None.

**Result:** `UndoStackDetails`

**Since:** 2.2.0

---

### UndoStack.undo {#doc_rpc_api_method_UndoStack_undo}

Undo the last command, then return the new state. If there is nothing to undo, the command fails with an error.

None.

**Result:** `UndoStackDetails`

**Since:** 2.2.0

---

### UndoStack.redo {#doc_rpc_api_method_UndoStack_redo}

Redo the last undone the last command, then return the new state. If there is nothing to redo, the command fails with an error.

None.

**Result:** `UndoStackDetails`

**Since:** 2.2.0

---

## Class RemoteLibrary {#doc_rpc_api_class_RemoteLibrary}

Allows to list, install, update and uninstall remote libraries.

### RemoteLibrary.list {#doc_rpc_api_method_RemoteLibrary_list}

Fetch all remote libraries. If the application already has the list ready, this method immediately returns all of them at once. If the list needs to be fetched from the configured servers first, this method streams the libraries in notifications.

None.

**Result:** `map`

**Fields:**

| Name | Type | Description |
|------|------|-------------|
| `libraries` | array of RemoteLibrarySummary | All remote libraries, in an unspecified (but stable) order. |

---

### RemoteLibrary.install {#doc_rpc_api_method_RemoteLibrary_install}

Install or update remote libraries in one batch. Note that dependent libraries will automatically be installed/updated too.

**Params:**

| Name | Type | Description |
|------|------|-------------|
| `libraries` | array of uuid_str | UUID of libraries to install. |

**Result:** `null`

---

### RemoteLibrary.uninstall {#doc_rpc_api_method_RemoteLibrary_uninstall}

Uninstall remote libraries in one batch. Note that dependent libraries will automatically be uninstalled too.
ATTENTION: Be careful with uninstalling libraries which are currently not available in any online repository, since this may lead to data loss which cannot be undone!

**Params:**

| Name | Type | Description |
|------|------|-------------|
| `libraries` | array of uuid_str | UUID of libraries to uninstall. |

**Result:** `null`

---

### RemoteLibrary.get {#doc_rpc_api_method_RemoteLibrary_get}

Get details about a remote library.

None.

**Result:** `RemoteLibraryDetails`

---

## Class WorkspaceDb {#doc_rpc_api_class_WorkspaceDb}

The library database of the currently opened workspace.
IMPORTANT: After the app startup, and after each modification of the local or remote libraries (install/remove/create/edit), call `wait()` before accessing the database again. Otherwise, the database may not reflect the changes yet, i.e. returns outdated information.

### WorkspaceDb.wait {#doc_rpc_api_method_WorkspaceDb_wait}

Wait for the library rescan to complete. If the database is already up-to-date, this method returns immediately. If a scan is currently in progress or scheduled, the method blocks until the scan finished. During the scan, progress notifications will be emitted.

None.

**Result:** `WorkspaceDbDetails`

