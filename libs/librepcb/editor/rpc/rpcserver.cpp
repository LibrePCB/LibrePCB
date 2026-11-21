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
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/rust-core/ffi.h>

#include <QThread>

#include <limits>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Local Functions
 ******************************************************************************/
namespace {

/// Hardcoded authentication token (development only!)
///
/// @note This makes the RPC server accept *any* local process, which is
///       fine during development but must never be shipped like this. It
///       will be replaced by a random token + the instance registry, see
///       section 9 of dev/plans/rpc-server.md.
const char* sDevToken = "librepcb-dev-token";

/// The protocol version, reported in the `initialize` response.
const char* sProtocolVersion = "1";

/// The name of the method which authenticates a connection.
///
/// @note Keep in sync with `METHOD_INITIALIZE` in
///       `rust-core/src/rpc_server/protocol.rs`!
const char* sMethodInitialize = "initialize";

/// Normalize a QVariant to the protocol's restricted value domain
/// (nil, bool, i64, f64, string, bytearray, list, map with string keys).
///
/// Plain C++ numeric types (int, uint, float, ...) are converted to the
/// corresponding protocol types. Anything outside the domain (e.g.
/// QDateTime or QColor) makes the normalization fail — such a result would
/// be unrepresentable on the wire, so the command would reply with an
/// internal error.
bool normalizeVariant(const QVariant& value, QVariant* out) {
  switch (value.userType()) {
    case QMetaType::UnknownType:  // An invalid QVariant is nil.
      *out = QVariant();
      return true;
    case QMetaType::Bool:
    case QMetaType::QString:
    case QMetaType::QByteArray:
    case QMetaType::LongLong:
    case QMetaType::Double:
      *out = value;
      return true;
    case QMetaType::Int:
    case QMetaType::Short:
    case QMetaType::SChar:
      *out = QVariant(static_cast<qlonglong>(value.toLongLong()));
      return true;
    case QMetaType::UInt:
    case QMetaType::UShort:
    case QMetaType::UChar:
      *out = QVariant(static_cast<qlonglong>(value.toUInt()));
      return true;
    case QMetaType::ULongLong:
    case QMetaType::ULong: {
      const qulonglong v = value.toULongLong();
      if (v > static_cast<qulonglong>(std::numeric_limits<qlonglong>::max())) {
        return false;  // Not representable as i64.
      }
      *out = QVariant(static_cast<qlonglong>(v));
      return true;
    }
    case QMetaType::Long:
      *out = QVariant(static_cast<qlonglong>(value.toLongLong()));
      return true;
    case QMetaType::Float:
      *out = QVariant(static_cast<double>(value.toFloat()));
      return true;
    case QMetaType::QVariantList: {
      QVariantList list;
      for (const QVariant& item : value.toList()) {
        QVariant normalized;
        if (!normalizeVariant(item, &normalized)) {
          return false;
        }
        list.append(normalized);
      }
      *out = list;
      return true;
    }
    case QMetaType::QVariantMap: {
      QVariantMap map;
      const QVariantMap src = value.toMap();
      for (auto it = src.constBegin(); it != src.constEnd(); ++it) {
        QVariant normalized;
        if (!normalizeVariant(it.value(), &normalized)) {
          return false;
        }
        map.insert(it.key(), normalized);
      }
      *out = map;
      return true;
    }
    default:
      return false;  // Not representable in the protocol.
  }
}

/// Serialize an error into a map of the protocol's value domain.
QVariantMap errorToVariant(rs::RpcErrorCode code, const QString& message,
                           const QVariantMap& properties) noexcept {
  QVariantMap error;
  error.insert("code", QVariant::fromValue(static_cast<qlonglong>(code)));
  error.insert("message", QVariant(message));
  for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
    error.insert(it.key(), it.value());
  }
  return error;
}

}  // namespace

/*******************************************************************************
 *  FFI Trampolines
 ******************************************************************************/

bool rpcServerDispatch(void* ctx, uint64_t connId, uint32_t msgid,
                       const QString* method, const QVariant* params) {
  Q_ASSERT(ctx && method && params);
  return static_cast<RpcServer*>(ctx)->dispatchFromWorkerThread(
      connId, msgid, *method, *params);
}

void rpcServerCancel(void* ctx, uint64_t connId, uint32_t msgid) {
  Q_ASSERT(ctx);
  static_cast<RpcServer*>(ctx)->cancelFromWorkerThread(connId, msgid);
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

RpcServer::RpcServer(GuiApplication& app) noexcept
  : QObject(&app), mApp(app), mServer(nullptr) {
  mCommands = buildCommandRegistry();

  // Create the server. Note that the token is hardcoded for now (see
  // sDevToken above) and validated by the C++ side in #handleInitialize.
  mServer = rs::ffi_rpc_server_new(&rpcServerDispatch, &rpcServerCancel, this);
  if (!mServer) {
    qCritical() << "Failed to create the RPC server.";
    return;
  }

  // Start listening on 127.0.0.1. A specific port can be forced with the
  // environment variable "LIBREPCB_RPC_PORT" (useful for testing).
  const quint16 port =
      static_cast<quint16>(qEnvironmentVariableIntValue("LIBREPCB_RPC_PORT"));
  quint16 actualPort = 0;
  if (rs::ffi_rpc_server_start(mServer, port, &actualPort)) {
    qInfo() << "RPC server now listening on 127.0.0.1, port" << actualPort;
  } else {
    qCritical() << "Failed to start the RPC server.";
    rs::ffi_rpc_server_delete(mServer);
    mServer = nullptr;
  }
}

RpcServer::~RpcServer() noexcept {
  // Invalidate all pending requests first: They must not call into the
  // (about to be deleted) Rust server anymore. The safety net in their
  // destructors is suppressed, i.e. clients just see the connection close.
  for (const auto& request : mPendingRequests) {
    request->invalidate();
  }
  mPendingRequests.clear();
  if (mServer) {
    rs::ffi_rpc_server_delete(mServer);  // Stops the server & joins threads.
    mServer = nullptr;
  }
}

/*******************************************************************************
 *  Request Handling
 ******************************************************************************/

RpcServer::Request::Request(GuiApplication& app, RpcServer& server,
                            uint64_t connId, uint32_t msgid,
                            QString method) noexcept
  : mApp(app),
    mServer(&server),
    mConnId(connId),
    mMsgid(msgid),
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

void RpcServer::Request::replySuccess(const QVariant& result) noexcept {
  if (mReplied) {
    return;  // A second reply is a bug in the command handler, ignore it.
  }
  QVariant normalized;
  if (!normalizeVariant(result, &normalized)) {
    qWarning() << "RPC command" << mMethod
               << "returned a value of an unsupported type.";
    replyError(rs::RpcErrorCode::InternalError,
               "The command returned an unsupported value type.");
    return;
  }
  sendReply(true, normalized);
}

void RpcServer::Request::replyError(rs::RpcErrorCode code,
                                    const QString& message,
                                    const QVariantMap& properties) noexcept {
  if (mReplied) {
    return;
  }
  QVariant error = errorToVariant(code, message, properties);
  QVariant normalized;
  if (!normalizeVariant(error, &normalized)) {
    normalized = errorToVariant(rs::RpcErrorCode::InternalError, message,
                                QVariantMap());  // Cannot fail.
  }
  sendReply(false, normalized);
}

void RpcServer::Request::emitMessage(const QVariant& payload) noexcept {
  if (!mServer || mReplied) {
    return;
  }
  QVariant normalized;
  if (!normalizeVariant(payload, &normalized)) {
    qWarning() << "RPC command" << mMethod
               << "emitted a message of an unsupported type.";
    return;
  }
  if (rs::RpcServer* rust = mServer->mServer) {
    rs::ffi_rpc_server_notify_conn(rust, mConnId, mMsgid, &normalized);
  }
}

void RpcServer::Request::sendReply(bool ok, const QVariant& value) noexcept {
  mReplied = true;
  if (mServer) {
    // Remove the request from the registry (the Rust server enforces the
    // "exactly one response" invariant anyway, but this keeps the registry
    // small).
    mServer->mPendingRequests.remove(requestKey(mConnId, mMsgid));
    if (rs::RpcServer* rust = mServer->mServer) {
      rs::ffi_rpc_server_reply(rust, mConnId, mMsgid, ok, &value);
    }
  }
}

bool RpcServer::dispatchFromWorkerThread(uint64_t connId, uint32_t msgid,
                                         const QString& method,
                                         const QVariant& params) noexcept {
  // Marshal the request to the GUI thread, asynchronously. Note that all
  // the values are captured by copy (cheap due to Qt's implicit sharing),
  // so they stay valid even if this thread continues right away.
  const bool invoked = QMetaObject::invokeMethod(
      this,
      [this, connId, msgid, method, params]() {
        handleRequest(connId, msgid, method, params);
      },
      Qt::QueuedConnection);
  if (!invoked) {
    qWarning() << "Failed to marshal RPC request to the GUI thread:" << method;
  }
  return invoked;
}

void RpcServer::cancelFromWorkerThread(uint64_t connId,
                                       uint32_t msgid) noexcept {
  const bool invoked = QMetaObject::invokeMethod(
      this, [this, connId, msgid]() { handleCancel(connId, msgid); },
      Qt::QueuedConnection);
  if (!invoked) {
    qWarning()
        << "Failed to marshal RPC cancel notification to the GUI thread.";
  }
}

void RpcServer::handleRequest(uint64_t connId, uint32_t msgid,
                              const QString& method,
                              const QVariant& params) noexcept {
  // The Rust server guarantees that params is either a map or nil.
  QVariantMap args;
  if (params.userType() == QMetaType::QVariantMap) {
    args = params.toMap();
  }

  // The `initialize` request is handled right here (not through the
  // command registry, since it's a protocol-level method): Validate the
  // token, mark the connection as (un)authenticated on the Rust side and
  // reply with the application information.
  if (method == sMethodInitialize) {
    handleInitialize(connId, msgid, args);
    return;
  }

  // Extract the receiver handle ("obj" is a documented convention, see the
  // API spec). It is an *opaque* value which is passed to the handlers
  // separately (so they never see it in their arguments) — only the
  // handlers know how to interpret it.
  QVariant obj;
  const auto objIt = args.constFind("obj");
  if (objIt != args.constEnd()) {
    obj = objIt.value();
    args.erase(objIt);
  }

  // Look up the command handler.
  const auto handler = mCommands.constFind(method);
  if (handler == mCommands.constEnd()) {
    replyError(connId, msgid, rs::RpcErrorCode::MethodNotFound,
               QString("Method not found: %1").arg(method));
    return;
  }

  // Create the request context and register it as pending.
  const auto request =
      QSharedPointer<Request>::create(mApp, *this, connId, msgid, method);
  mPendingRequests.insert(requestKey(connId, msgid), request);
  try {
    handler.value()(request, obj, args);
  } catch (const Exception& e) {
    request->replyError(rs::RpcErrorCode::CommandFailed,
                        QString("Command failed: %1").arg(e.getMsg()));
  } catch (const std::exception& e) {
    request->replyError(rs::RpcErrorCode::CommandFailed,
                        QString("Command failed: %1").arg(e.what()));
  } catch (...) {
    request->replyError(rs::RpcErrorCode::CommandFailed,
                        "Command failed with unknown error.");
  }
}

void RpcServer::handleInitialize(uint64_t connId, uint32_t msgid,
                                 const QVariantMap& args) noexcept {
  // Validate the token.
  const QVariant token = args.value("token");
  const bool authenticated = (token.userType() == QMetaType::QString) &&
      (token.toString() == QString::fromUtf8(sDevToken));
  if (mServer) {
    rs::ffi_rpc_server_set_authenticated(mServer, connId, authenticated);
  }
  if (authenticated) {
    QVariantMap result;
    result.insert("protocolVersion", QString::fromUtf8(sProtocolVersion));
    result.insert("appVersion", Application::getVersion());
    result.insert("pid",
                  static_cast<qlonglong>(QCoreApplication::applicationPid()));
    result.insert("workspace", mApp.getWorkspace().getPath().toStr());
    replySuccess(connId, msgid, result);
  } else {
    replyError(connId, msgid, rs::RpcErrorCode::Unauthenticated,
               "Invalid or missing token");
  }
}

void RpcServer::handleCancel(uint64_t connId, uint32_t msgid) noexcept {
  // Note: The Rust server only forwards cancels for requests which are
  // still pending, but the request might have been replied in the
  // meantime (between marshalling and executing this call), so the lookup
  // may just fail — that's fine, then the work is already done.
  const auto request = mPendingRequests.constFind(requestKey(connId, msgid));
  if (request == mPendingRequests.constEnd()) {
    return;
  }
  request.value()->setCancelled();
}

void RpcServer::notifyEvent(const QString& method,
                            const QVariant& params) noexcept {
  if (mServer) {
    rs::ffi_rpc_server_notify_all(mServer, &method, &params);
  }
}

void RpcServer::replySuccess(uint64_t connId, uint32_t msgid,
                             const QVariant& result) noexcept {
  // Reply directly, without a #Request object (used for `initialize`).
  if (mServer) {
    QVariant normalized;
    if (!normalizeVariant(result, &normalized)) {
      qWarning() << "RPC initialize returned a value of an unsupported type.";
      replyError(connId, msgid, rs::RpcErrorCode::InternalError,
                 "The command returned an unsupported value type.");
      return;
    }
    rs::ffi_rpc_server_reply(mServer, connId, msgid, true, &normalized);
  }
}

void RpcServer::replyError(uint64_t connId, uint32_t msgid,
                           rs::RpcErrorCode code,
                           const QString& message) noexcept {
  if (mServer) {
    QVariant error = errorToVariant(code, message, QVariantMap());
    QVariant normalized;
    if (!normalizeVariant(error, &normalized)) {
      normalized = errorToVariant(rs::RpcErrorCode::InternalError, message,
                                  QVariantMap());  // Cannot fail.
    }
    rs::ffi_rpc_server_reply(mServer, connId, msgid, false, &normalized);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
