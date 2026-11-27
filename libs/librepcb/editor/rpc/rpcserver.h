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
#include "rpcobjectregistry.h"
#include "rpcrequest.h"

#include <librepcb/core/utils/rusthandle.h>
#include <librepcb/core/utils/signalslot.h>
#include <librepcb/rust-core/ffi.h>

#include <QtCore>

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
 *   in `dev/plans/rpc-server.md` and `dev/rpc/rpc.yml`).
 * - It provides the command dispatcher which marshals incoming commands to
 *   the GUI thread (via `Qt::QueuedConnection`, asynchronously!) and
 *   dispatches them to the generated command classes (see `commands.cpp`).
 * - Commands may reply synchronously (from within the handler) or later
 *   (e.g. after a long-running task finished), optionally emitting
 *   request-scoped messages (e.g. progress) in the meantime.
 * - It pushes application events (e.g. "project_opened") to all clients.
 *
 * @note The authentication of external clients (Ed25519 keys confirmed by
 *       the user) is implemented in the `authenticate` handler, see
 *       `api/protocolrpc.cpp`. The spawned-process token mechanism and the
 *       endpoint file for discovery are planned, see the RPC server
 *       documentation.
 */
class RpcServer final : public QObject {
  Q_OBJECT

public:
  // Types

  // Constructors / Destructor
  RpcServer() = delete;
  RpcServer(const RpcServer& other) = delete;

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
   * @brief Get the registry of RPC object handles
   *
   * Its lifetime is bound to the server, i.e. all handles become invalid
   * when the server is stopped (but handle values are never reused).
   */
  RpcObjectRegistry& getObjectRegistry() noexcept { return mObjectRegistry; }

private:  // Methods
  /// Called on the GUI thread: Extract the "obj" key, look up the command
  /// handler and execute it (or reply with an error).
  void handleRequest(uint32_t connId, uint32_t msgId, const QString& method,
                     const QVariant& args) noexcept;

  /// Called on the GUI thread: Mark a pending request as cancelled.
  void handleCancel(uint32_t connId, uint32_t msgId) noexcept;

  /// Called by an ::librepcb::editor::RpcRequest on any event
  void requestEvent(const RpcRequest& request, RpcRequest::Event event,
                    uint32_t status, const QVariant& value) noexcept;

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

  /// The TCP port the server is listening on (coming from Rust side)
  uint16_t mPort;

  /// The FFI handle of the (Rust implemented) server
  RustHandle<rs::RpcServer> mHandle;

  /// The registry of RPC object handles
  RpcObjectRegistry mObjectRegistry;

  /// Pending (dispatched, not yet replied) requests, by connection+msgId
  QHash<uint64_t, std::shared_ptr<RpcRequest>> mPendingRequests;

  // Slots
  RpcRequest::OnEventSlot mRequestEventSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
