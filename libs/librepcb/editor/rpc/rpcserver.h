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
#include "rpcrequest.h"

#include <librepcb/core/utils/rusthandle.h>
#include <librepcb/core/utils/signalslot.h>
#include <librepcb/rust-core/ffi.h>

#include <QtCore>

#include <functional>
#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Project;

namespace editor {

class GuiApplication;
class ProjectEditor;

/*******************************************************************************
 *  Class RpcServer
 ******************************************************************************/

/**
 * @brief The RPC server to control the application with external tools
 *
 * This class integrates the Rust RPC server (rust-core module `rpc_server`)
 * into the LibrePCB application:
 *
 * - It starts and stops the (Rust implemented) server, which listens on
 *   127.0.0.1 for msgpack-encoded RPC commands (see the specification
 *   in `dev/plans/rpc-server.md` and `share/librepcb/rpc/rpc.yaml`).
 * - It provides the command dispatcher which marshals incoming commands to
 *   the GUI thread (via `Qt::QueuedConnection`, asynchronously!) and
 *   dispatches them to the command handlers registered in `commands.cpp`.
 * - Commands may reply synchronously (from within the handler) or later
 *   (e.g. after a long-running task finished), optionally emitting
 *   request-scoped messages (e.g. progress) in the meantime.
 * - It pushes application events (e.g. "project_opened") to all clients.
 *
 * @note The token is currently hardcoded (development only!) and thus the
 *       server accepts any local process. To be replaced by a random token
 *       together with the instance registry, see the plan section 9.
 */
class RpcServer final : public QObject {
  Q_OBJECT

public:
  // Types

  // Note: The RPC error codes are defined by the Rust server (see
  // `RpcErrorCode` in `rust-core/src/rpc_server/protocol.rs`, exposed
  // through `ffi.h` as `rs::RpcErrorCode`) and are used directly from
  // there — no duplication.

  /// A command handler, executed on the GUI thread. It must reply the
  /// request (synchronously or asynchronously, see #RpcRequest).
  using RpcHandler = std::function<void(RpcRequest& request)>;

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
  ~RpcServer() noexcept override = default;

  /**
   * @brief Get all registered command handlers, by method name
   *
   * @note This is a static method on purpose, i.e. it does not require an
   *       application or a running server. It allows testing the registry
   *       without launching the whole application.
   */
  static QHash<QString, RpcHandler> buildCommandRegistry() noexcept;

  /**
   * @brief Send the response for a request to the client
   *
   * Called by the #RpcRequest when it is replied. Never blocks. If the
   * request is unknown or already replied, the response is silently
   * dropped by the Rust server.
   *
   * @param connId  The connection ID of the request.
   * @param msgId   The msg_id of the request.
   * @param ok      Whether the value is a result or an error map.
   * @param value   The result or error map.
   */
  void sendResponse(uint32_t connId, uint32_t msgId, bool ok,
                    const QVariant& value) noexcept;

  /**
   * @brief Send a request-scoped message (e.g. progress) to the client
   *
   * Called by the #RpcRequest while it is pending. Never blocks. The
   * message is silently dropped by the Rust server if the request is not
   * (or no longer) pending.
   *
   * @param connId  The connection ID of the request.
   * @param msgId   The msg_id of the request.
   * @param payload The message payload.
   */
  void sendNotification(uint32_t connId, uint32_t msgId,
                        const QVariant& payload) noexcept;

private:  // Methods
  /// Called on the GUI thread: Extract the "obj" key, look up the command
  /// handler and execute it (or reply with an error).
  void handleRequest(uint32_t connId, uint32_t msgId, const QString& method,
                     const QVariant& args) noexcept;

  /// Called on the GUI thread: Mark a pending request as cancelled.
  void handleCancel(uint32_t connId, uint32_t msgId) noexcept;

  /// Called on the GUI thread: Handle an `initialize` request — validate
  /// the token, mark the connection as (un)authenticated on the Rust side
  /// and reply with the application information.
  void handleInitialize(RpcRequest& request) noexcept;

  /// Called by an #RpcRequest when it was replied: Remove it from the
  /// pending-request registry.
  void requestFinished(const RpcRequest& request) noexcept;

  /// Key for a pending request in the registry.
  static uint64_t requestKey(uint32_t connId, uint32_t msgId) noexcept {
    return (static_cast<uint64_t>(connId) << 32) | msgId;
  }

  static void onRequestCallback(void* ctx, uint32_t connId, uint32_t msgId,
                                const QString* method, const QVariant* params);
  static void onCancelCallback(void* ctx, uint32_t connId, uint32_t msgId);

private:  // Data
  /// The application to dispatch the commands to
  GuiApplication& mApp;

  /// All registered command handlers, by method name
  QHash<QString, RpcHandler> mCommands;

  /// The TCP port the server is listening on (coming from Rust side)
  uint16_t mPort;

  /// The FFI handle of the (Rust implemented) server
  RustHandle<rs::RpcServer> mHandle;

  /// Reacts on replied requests (removes them from the registry)
  RpcRequest::OnFinishedSlot mRequestFinishedSlot;

  /// Pending (dispatched, not yet replied) requests, by connection+msgId
  QHash<uint64_t, std::shared_ptr<RpcRequest>> mPendingRequests;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
