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
    mRequestNotificationSlot(*this, &RpcServer::requestNotification),
    mRequestFinishedSlot(*this, &RpcServer::requestFinished) {
  mCommands.insert(
      "initialize",
      std::bind(&RpcServer::handleInitialize, this, std::placeholders::_1));
  mCommands.insert(buildCommandRegistry());
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
  const auto request = std::make_shared<RpcRequest>(mApp, *this, connId, msgId,
                                                    method, args.toMap());
  request->onNotification.attach(mRequestNotificationSlot);
  request->onFinished.attach(mRequestFinishedSlot);
  mPendingRequests.insert(requestKey(connId, msgId), request);

  // Look up the command handler.
  const auto handler = mCommands.value(method);
  if (!handler) {
    request->sendError(rs::RpcErrorCode::MethodNotFound,
                       QString("Method not found: %1").arg(method));
    return;
  }

  // Run the command handler.
  try {
    handler(*request);
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
    request->cancel();
  }
}

void RpcServer::handleInitialize(RpcRequest& request) noexcept {
  const QString clientName =
      request.getArgs().value("name").toString().simplified().left(32);
  const QVariant token = request.getArgs().value("token");

  auto createResponse = [this, clientName](QString token) {
    static const Uuid session = Uuid::createRandom();

    if (token.isEmpty()) {
      token = Uuid::createRandom().toStr();
      mAuthenticatedTokens.insert(token);
      qInfo() << "RPC client" << clientName << "connected.";
    }

    QVariantMap result;
    result.insert("format", qlonglong(1));
    result.insert("server", "librepcb");
    result.insert("version", Application::getVersion());
    result.insert("session", session.toStr());
    result.insert("workspace", mApp.getWorkspace().getPath().toStr());
    result.insert("token", token);
    return result;
  };

  if (!token.isNull()) {
    const bool authenticated = mAuthenticatedTokens.contains(token.toString());
    rs::ffi_rpc_server_set_authenticated(*mHandle, request.getConnId(),
                                         authenticated);
    if (authenticated) {
      request.sendSuccess(createResponse(token.toString()));
    } else {
      request.sendError(rs::RpcErrorCode::AccessDenied,
                        "Invalid or revoked access token.");
    }
  } else {
    auto notification = std::make_shared<Notification>(
        ui::NotificationType::Warning, tr("External RPC Connection Request"),
        tr("An external application, which identifies itself as '%1', wants to "
           "connect through the RPC interface to this process for the first "
           "time. Only accept this if you trust that application and the "
           "request was triggered by yourself.")
            .arg(clientName),
        tr("Accept Connection"), QString(), true);
    connect(notification.get(), &Notification::buttonClicked, &request,
            [&request, createResponse]() {
              request.sendSuccess(createResponse(QString()));
            });
    connect(notification.get(), &Notification::destroyed, &request,
            [&request]() {
              request.sendError(rs::RpcErrorCode::AccessDenied,
                                "Access request denied.");
            });
    connect(&request, &RpcRequest::cancelled, notification.get(),
            &Notification::dismiss);
    connect(&request, &RpcRequest::destroyed, notification.get(),
            &Notification::dismiss);
    mApp.getNotifications().push(notification);
  }
}

void RpcServer::requestNotification(const RpcRequest& request,
                                    const QVariant& value) noexcept {
  rs::ffi_rpc_server_notify(*mHandle, request.getConnId(), request.getMsgId(),
                            &value);
}

void RpcServer::requestFinished(const RpcRequest& request, bool ok,
                                const QVariant& value) noexcept {
  rs::ffi_rpc_server_reply(*mHandle, request.getConnId(), request.getMsgId(),
                           ok, &value);
  mPendingRequests.remove(requestKey(request.getConnId(), request.getMsgId()));
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
