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

/// Serialize an error into a map of the protocol's value domain.
static QVariantMap errorToVariant(rs::RpcErrorCode code, const QString& message,
                                  const QVariantMap& properties) noexcept {
  QVariantMap error;
  error.insert("code", QVariant::fromValue(static_cast<qlonglong>(code)));
  error.insert("message", QVariant(message));
  for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
    error.insert(it.key(), it.value());
  }
  return error;
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
    mHandle(construct(&onRequestCallback, &onCancelCallback, this, mPort)) {
  mCommands = buildCommandRegistry();
  qInfo().nospace() << "RPC server is listening on 127.0.0.1, port " << mPort
                    << ".";
}

RpcServer::~RpcServer() noexcept {
  // Invalidate all pending requests first: They must not call into the
  // (about to be deleted) Rust server anymore. The safety net in their
  // destructors is suppressed, i.e. clients just see the connection close.
  for (const auto& request : mPendingRequests) {
    request->invalidate();
  }
  mPendingRequests.clear();
}

/*******************************************************************************
 *  Request Handling
 ******************************************************************************/

RpcServer::Request::Request(GuiApplication& app, RpcServer& server,
                            uint32_t connId, uint32_t msgId,
                            QString method) noexcept
  : mApp(app),
    mServer(&server),
    mConnId(connId),
    mMsgid(msgId),
    mMethod(std::move(method)),
    mReplied(false),
    mCancelled(false) {
}

RpcServer::Request::~Request() noexcept {
  // Safety net to guarantee the "exactly one response per request" invariant
  // even if a handler forgot to reply.
  if (!mReplied) {
    if (mServer) {
      qWarning() << "RPC command" << mMethod
                 << "did not reply, answering with an error.";
      sendReply(false,
                errorToVariant(rs::RpcErrorCode::InternalError,
                               "The command did not reply.", QVariantMap()));
    } else {
      qWarning() << "RPC command" << mMethod
                 << "did not reply, but the RPC server is already destroyed.";
    }
  }
}

void RpcServer::Request::sendNotification(const QVariant& payload) noexcept {
  if (!mServer || mReplied) {
    return;
  }
  rs::ffi_rpc_server_notify(*mServer->mHandle, mConnId, mMsgid, &payload);
}

void RpcServer::Request::sendSuccess(const QVariant& result) noexcept {
  if (mReplied) {
    return;  // A second reply is a bug in the command handler, ignore it.
  }
  sendReply(true, result);
}

void RpcServer::Request::sendError(rs::RpcErrorCode code,
                                   const QString& message,
                                   const QVariantMap& properties) noexcept {
  if (mReplied) {
    return;
  }
  sendReply(false, errorToVariant(code, message, properties));
}

void RpcServer::Request::sendReply(bool ok, const QVariant& value) noexcept {
  mReplied = true;
  if (mServer) {
    // Remove the request from the registry (the Rust server enforces the
    // "exactly one response" invariant anyway, but this keeps the registry
    // small).
    mServer->mPendingRequests.remove(requestKey(mConnId, mMsgid));
    rs::ffi_rpc_server_reply(*mServer->mHandle, mConnId, mMsgid, ok, &value);
  }
}

void RpcServer::handleRequest(uint32_t connId, uint32_t msgId,
                              const QString& method,
                              const QVariant& args) noexcept {
  const auto request =
      std::make_shared<Request>(mApp, *this, connId, msgId, method);

  // The `initialize` request is handled right here (not through the
  // command registry, since it's a protocol-level method).
  if (method == "initialize") {
    handleInitialize(connId, msgId, args.toMap());
    return;
  }

  // Look up the command handler.
  const auto handler = mCommands.constFind(method);
  if (handler == mCommands.constEnd()) {
    replyError(connId, msgId, rs::RpcErrorCode::MethodNotFound,
               QString("Method not found: %1").arg(method));
    return;
  }

  // Create the request context and register it as pending.

  mPendingRequests.insert(requestKey(connId, msgId), request);
  try {
    // The Rust server guarantees that params is either a map or nil.
    handler.value()(*request, args.toMap());
  } catch (const Exception& e) {
    request->sendError(rs::RpcErrorCode::CommandFailed,
                       QString("Command failed: %1").arg(e.getMsg()));
  } catch (const std::exception& e) {
    request->sendError(rs::RpcErrorCode::CommandFailed,
                       QString("Command failed: %1").arg(e.what()));
  } catch (...) {
    request->sendError(rs::RpcErrorCode::CommandFailed,
                       "Command failed with unknown error.");
  }
}

void RpcServer::handleInitialize(uint32_t connId, uint32_t msgId,
                                 const QVariantMap& args) noexcept {
  // Validate the token.
  const QVariant token = args.value("token");
  const bool authenticated = (token.userType() == QMetaType::QString) &&
      (token.toString() == "librepcb-dev-token");
  rs::ffi_rpc_server_set_authenticated(*mHandle, connId, authenticated);
  if (authenticated) {
    QVariantMap result;
    result.insert("appVersion", Application::getVersion());
    result.insert("pid",
                  static_cast<qlonglong>(QCoreApplication::applicationPid()));
    result.insert("workspace", mApp.getWorkspace().getPath().toStr());
    replySuccess(connId, msgId, result);
  } else {
    replyError(connId, msgId, rs::RpcErrorCode::Unauthenticated,
               "Invalid or missing token");
  }
}

void RpcServer::handleCancel(uint32_t connId, uint32_t msgId) noexcept {
  // Note: The Rust server only forwards cancels for requests which are
  // still pending, but the request might have been replied in the
  // meantime (between marshalling and executing this call), so the lookup
  // may just fail — that's fine, then the work is already done.
  const auto request = mPendingRequests.constFind(requestKey(connId, msgId));
  if (request == mPendingRequests.constEnd()) {
    return;
  }
  request.value()->setCancelled();
}

void RpcServer::replySuccess(uint32_t connId, uint32_t msgId,
                             const QVariant& result) noexcept {
  // Reply directly, without a #Request object (used for `initialize`).
  rs::ffi_rpc_server_reply(*mHandle, connId, msgId, true, &result);
}

void RpcServer::replyError(uint32_t connId, uint32_t msgId,
                           rs::RpcErrorCode code,
                           const QString& message) noexcept {
  QVariant error = errorToVariant(code, message, QVariantMap());
  rs::ffi_rpc_server_reply(*mHandle, connId, msgId, false, &error);
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
