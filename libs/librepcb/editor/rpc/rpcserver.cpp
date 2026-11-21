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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "rpcserver.h"

#include "../guiapplication.h"

#include <librepcb/core/application.h>
#include <librepcb/core/exceptions.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/rust-core/ffi.h>

#include <QThread>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Local Functions
 ******************************************************************************/

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

static RustHandle<rs::RpcServer> construct(rs::RpcOnRequestCallback onRequest,
                                           rs::RpcOnCancelCallback onCancel,
                                           void* ctx, uint16_t& port) {
  if (auto obj = rs::ffi_rpc_server_new(onRequest, onCancel, ctx, &port)) {
    return RustHandle<rs::RpcServer>(*obj, &rs::ffi_rpc_server_delete);
  } else {
    throw RuntimeError(__FILE__, __LINE__, "Failed to create RpcServer");
  }
}

RpcServer::RpcServer(GuiApplication& app) noexcept
  : QObject(&app),
    mApp(app),
    mPort(0),  // Will be initialized by construct()
    mHandle(construct(&onRequestCallback, &onCancelCallback, this, mPort)),
    mRequestFinishedSlot(*this, &RpcServer::requestFinished) {
  mCommands.insert(
      "initialize",
      std::bind(&RpcServer::handleInitialize, this, std::placeholders::_1));
  mCommands.insert(buildCommandRegistry());
  qInfo().nospace() << "RPC server is listening on 127.0.0.1, port " << mPort
                    << ".";
}

/*******************************************************************************
 *  Command Handling
 ******************************************************************************/

void RpcServer::handleRequest(uint32_t connId, uint32_t msgId,
                              const QString& method,
                              const QVariant& args) noexcept {
  // Create the request context and register it as pending.
  // Note: The Rust server guarantees that params is either a map or nil.
  const auto request = std::make_shared<RpcRequest>(mApp, *this, connId, msgId,
                                                    method, args.toMap());
  request->onFinished.attach(mRequestFinishedSlot);
  mPendingRequests.insert(requestKey(connId, msgId), request);

  // Look up the command handler.
  const auto handler = mCommands.constFind(method);
  if (handler == mCommands.constEnd()) {
    request->sendError(rs::RpcErrorCode::MethodNotFound,
                       QString("Method not found: %1").arg(method));
    return;  // Do not keep request object.
  }

  // Run the command handler.
  try {
    handler.value()(*request);
  } catch (const std::exception& e) {
    request->sendError(rs::RpcErrorCode::CommandFailed,
                       QString("Command failed: %1").arg(e.what()));
  } catch (...) {
    request->sendError(rs::RpcErrorCode::CommandFailed,
                       "Command failed with unknown error.");
  }
}

void RpcServer::handleCancel(uint32_t connId, uint32_t msgId) noexcept {
  // Note: The Rust server only forwards cancels for requests which are
  // still pending, but the request might have been replied in the
  // meantime (between marshalling and executing this call), so the lookup
  // may just fail — that's fine, then the work is already done.
  if (auto request = mPendingRequests.value(requestKey(connId, msgId))) {
    request->setCancelled();
  }
}

void RpcServer::handleInitialize(RpcRequest& request) noexcept {
  const QVariant token = request.getArgs().value("token");
  const bool authenticated = (token.toString() == "librepcb-dev-token");
  rs::ffi_rpc_server_set_authenticated(*mHandle, request.getConnId(),
                                       authenticated);
  if (!authenticated) {
    request.sendError(rs::RpcErrorCode::Unauthenticated,
                      "Invalid or missing token");
    return;
  }

  QVariantMap result;
  result.insert("server", "librepcb");
  result.insert("version", Application::getVersion());
  result.insert("workspace", mApp.getWorkspace().getPath().toStr());
  request.sendSuccess(result);
}

void RpcServer::requestFinished(const RpcRequest& request) noexcept {
  // Copy the IDs first: Removing the request from the registry drops the
  // server's reference to it, which might be the last one, destroying the
  // object while this method is still running.
  const uint32_t connId = request.getConnId();
  const uint32_t msgId = request.getMsgId();
  mPendingRequests.remove(requestKey(connId, msgId));
}

void RpcServer::sendResponse(uint32_t connId, uint32_t msgId, bool ok,
                             const QVariant& value) noexcept {
  rs::ffi_rpc_server_reply(*mHandle, connId, msgId, ok, &value);
}

void RpcServer::sendNotification(uint32_t connId, uint32_t msgId,
                                 const QVariant& payload) noexcept {
  rs::ffi_rpc_server_notify(*mHandle, connId, msgId, &payload);
}

// ATTENTION: This function is called from other threads (from Rust side)!
void RpcServer::onRequestCallback(void* ctx, uint32_t connId, uint32_t msgId,
                                  const QString* method,
                                  const QVariant* params) {
  Q_ASSERT(ctx && method && params);
  auto* server = static_cast<RpcServer*>(ctx);
  if (!QMetaObject::invokeMethod(
          server,
          [server, connId, msgId, method = *method, params = *params]() {
            server->handleRequest(connId, msgId, method, params);
          },
          Qt::QueuedConnection)) {
    qCritical() << "Failed to marshal RPC request to the GUI thread.";
  }
}

// ATTENTION: This function is called from other threads (from Rust side)!
void RpcServer::onCancelCallback(void* ctx, uint32_t connId, uint32_t msgId) {
  Q_ASSERT(ctx);
  auto* server = static_cast<RpcServer*>(ctx);
  if (!QMetaObject::invokeMethod(
          server,
          [server, connId, msgId]() { server->handleCancel(connId, msgId); },
          Qt::QueuedConnection)) {
    qCritical() << "Failed to marshal RPC cancel request to the GUI thread.";
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
