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
#include "../notification.h"
#include "../notificationsmodel.h"
#include "api/rpcerrors.h"

#include <librepcb/core/application.h>
#include <librepcb/core/exceptions.h>
#include <librepcb/core/types/uuid.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/rust-core/ffi.h>

#include <QThread>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

// Defined in commands.cpp (a generated file): Dispatches an RPC command
// by its method name to the corresponding generated command class. Throws
// an RpcError if the method was not found.
namespace rpc {
void rpcDispatchCommand(GuiApplication& app, RpcObjectRegistry& registry,
                        RpcRequest& request);
}

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
    mRequestEventSlot(*this, &RpcServer::requestEvent) {
  qInfo().nospace() << "RPC server is listening on 127.0.0.1, port " << mPort
                    << ".";
}

RpcServer::~RpcServer() noexcept {
}

/*******************************************************************************
 *  Command Handling
 ******************************************************************************/

void RpcServer::handleRequest(uint32_t connId, uint32_t msgId,
                              const QString& method,
                              const QVariant& args) noexcept {
  // Create the request context and register it as pending.
  // Note: The Rust server guarantees that params is either a map or nil.
  const auto request =
      std::make_shared<RpcRequest>(connId, msgId, method, args.toMap());
  request->onEvent.attach(mRequestEventSlot);
  mPendingRequests.insert(requestKey(connId, msgId), request);

  // Dispatch the command to its handler (generated code, see commands.cpp)
  // and translate thrown errors into error responses.
  try {
    rpc::rpcDispatchCommand(mApp, mObjectRegistry, *request);
  } catch (const RpcError& e) {
    request->sendError(e);
  } catch (const std::exception& e) {
    request->sendError(rpc::RpcCommandFailed(e.what()));
  } catch (...) {
    request->sendError(rpc::RpcCommandFailed());
  }

  // Safety net: If the command handler did not respond, and did not make
  // the request async (which requires a supervisor object to be registered),
  // respond with an error now instead of risking the request to be hanging
  // forever.
  if ((!request->isFinished()) && (!request->isAsync())) {
    request->sendError(rpc::RpcInternalError("No supervisor registered."));
  }
}

void RpcServer::handleCancel(uint32_t connId, uint32_t msgId) noexcept {
  // Note: The Rust server only forwards cancels for requests which are
  // still pending, but the request might have been replied in the
  // meantime (between marshalling and executing this call), so the lookup
  // may just fail — that's fine, then the work is already done.
  if (auto request = mPendingRequests.value(requestKey(connId, msgId))) {
    request->cancel();
  }
}

void RpcServer::requestEvent(const RpcRequest& request, RpcRequest::Event event,
                             uint32_t connId, uint32_t msgId, uint32_t status,
                             const QVariant& value) noexcept {
  Q_UNUSED(request);
  switch (event) {
    case RpcRequest::Event::Notification:
      rs::ffi_rpc_server_notify(*mHandle, connId, msgId, &value);
      break;
    case RpcRequest::Event::Reply:
      rs::ffi_rpc_server_reply(*mHandle, connId, msgId, status, &value);
      mPendingRequests.remove(requestKey(connId, msgId));
      break;
  }
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
