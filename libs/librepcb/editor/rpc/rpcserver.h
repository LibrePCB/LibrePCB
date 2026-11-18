/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBREPCB_EDITOR_RPCSERVER_H
#define LIBREPCB_EDITOR_RPCSERVER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/exceptions.h>

#include <QJsonObject>
#include <QtCore>

#include <atomic>
#include <functional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Project;

namespace rs {
struct RpcServer;
}

namespace editor {

class GuiApplication;
class ProjectEditor;

/// FFI trampoline which dispatches commands from the Rust RPC server to
/// the #librepcb::editor::RpcServer object passed as context.
///
/// @param ctx      The RpcServer object (as passed to ffi_rpc_server_new()).
/// @param method   The method name, e.g. "project/list".
/// @param params   The raw JSON of the request params.
/// @param response The serialized response (result or error).
///
/// @return Whether the command succeeded.
extern "C" bool rpcServerDispatch(void* ctx, const QString* method,
                                  const QString* params, QString* response);

/*******************************************************************************
 *  Class RpcServer
 ******************************************************************************/

/**
 * @brief The RPC server to control the application with external tools
 *
 * This class integrates the Rust JSON-RPC server (rust-core module
 * `rpc_server`) into the LibrePCB application:
 *
 * - It starts and stops the (Rust implemented) server, which listens on
 *   127.0.0.1 for JSON-RPC 2.0 commands (see `dev/plans/rpc-server.md`).
 * - It provides the command dispatcher which marshals incoming commands to
 *   the GUI thread (via `Qt::BlockingQueuedConnection`) and dispatches them
 *   to the command handlers registered in `commands.cpp`.
 * - It serves the OpenRPC API schema (from `share/librepcb/rpc/api.json`)
 *   via the `rpc.discover` method.
 *
 * @note The token is currently hardcoded (development only!) and thus the
 *       server accepts any local process. To be replaced by a random token
 *       together with the instance registry, see the plan section 9.
 *
 * @warning Code running on the GUI thread must never synchronously call back
 *          into this server (that would deadlock)! #notifyEvent() is safe
 *          since it is non-blocking. All other interaction must happen
 *          through the command handlers.
 */
class RpcServer final : public QObject {
  Q_OBJECT

public:
  // Types

  /**
   * @brief Application-specific JSON-RPC error codes
   *
   * @note Keep in sync with the `error_code` module in
   *       `rust-core/src/rpc_server/protocol.rs`!
   */
  enum ErrorCode {
    Unauthenticated = -32001,
    CommandFailed = -32002,
    ProjectNotOpen = -32003,
    ServerShuttingDown = -32004,
  };

  /// JSON-RPC error codes defined by the specification
  enum StandardErrorCode { MethodNotFound = -32601, InvalidParams = -32602 };

  /**
   * @brief The result of a command handler
   */
  struct RpcResult final {
    /// Whether the command succeeded
    bool ok = false;
    /// The result value (only valid if #ok is `true`)
    QJsonObject result;
    /// The error code (only valid if #ok is `false`)
    int errorCode = 0;
    /// The error message (only valid if #ok is `false`)
    QString errorMessage;

    /// Create a successful result
    static RpcResult success(QJsonObject result = QJsonObject()) noexcept {
      return RpcResult{true, result, 0, QString()};
    }

    /// Create a failed result
    static RpcResult failure(int code, const QString& message) noexcept {
      return RpcResult{false, QJsonObject(), code, message};
    }
  };

  /// A command handler, executed on the GUI thread
  using RpcHandler =
      std::function<RpcResult(GuiApplication&, const QJsonObject&)>;

  // Constructors / Destructor
  RpcServer() = delete;
  RpcServer(const RpcServer& other) = delete;
  RpcServer& operator=(const RpcServer& rhs) = delete;

  /**
   * @brief Start the RPC server (listening on 127.0.0.1)
   *
   * @param app   The application to dispatch the commands to.
   */
  explicit RpcServer(GuiApplication& app) noexcept;

  /**
   * @brief Stop the RPC server (must be called while the application is
   *        still fully alive, i.e. as the first step of the teardown)
   */
  ~RpcServer() noexcept override;

  /**
   * @brief Push an event (a JSON-RPC notification) to all connected clients
   *
   * This is safe to be called from the GUI thread since it never blocks.
   *
   * @param method    The event name, e.g. "event/projectOpened".
   * @param params    The raw JSON of the event params (may be empty).
   */
  void notifyEvent(const QString& method, const QString& params) noexcept;

  /**
   * @brief Get all registered command handlers, by method name
   *
   * @note This is a static method on purpose, i.e. it does not require an
   *       application or a running server. It is used by a unit test to
   *       verify that the methods are in sync with the API schema.
   */
  static QHash<QString, RpcHandler> buildCommandRegistry() noexcept;

private:  // Methods
  /// @brief The FFI trampoline needs access to the private dispatch methods
  friend bool rpcServerDispatch(void* ctx, const QString* method,
                                const QString* params, QString* response);

  /**
   * @brief Dispatch a command request (on the calling thread)
   *
   * Called by the FFI trampoline, i.e. on one of the Rust connection
   * threads. Marshals the command to the GUI thread, blocking until the
   * command was executed (or the server is shutting down).
   *
   * @param method    The method name, e.g. "project/list".
   * @param params    The raw JSON of the request params.
   * @param response  The serialized response (result or error, see
   *                  `RpcDispatchFn` in `ffi/rpc_server_ffi.rs`).
   *
   * @return Whether the command succeeded.
   */
  bool dispatchFromWorkerThread(const QString& method, const QString& params,
                                QString& response) noexcept;

  /**
   * @brief Execute a command request (on the GUI thread)
   *
   * @param method    The method name.
   * @param params    The raw JSON of the request params.
   * @param response  The serialized response (result or error).
   *
   * @return Whether the command succeeded.
   */
  bool dispatch(const QString& method, const QString& params,
                QString& response) noexcept;

  /**
   * @brief Serialize an error response
   *
   * @param response  The resulting JSON object (as serialized string).
   * @param code      The error code.
   * @param message   The error message.
   */
  static void writeErrorResponse(QString& response, int code,
                                 const QString& message) noexcept;

private:  // Data
  /// The application to dispatch the commands to
  GuiApplication& mApp;

  /// The FFI handle of the (Rust implemented) server, if running
  rs::RpcServer* mServer;

  /// All registered command handlers, by method name
  QHash<QString, RpcHandler> mCommands;

  /// Whether the server is currently shutting down (new commands are
  /// rejected instead of being marshalled to the GUI thread)
  std::atomic<bool> mShuttingDown;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
