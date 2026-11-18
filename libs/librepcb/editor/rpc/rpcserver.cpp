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
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/rust-core/ffi.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Local Data
 ******************************************************************************/
namespace {

/// Hardcoded authentication token (development only!)
///
/// @note This makes the RPC server accept *any* local process, which is
///       fine during development but must never be shipped like this. It
///       will be replaced by a random token + the instance registry, see
///       section 9 of dev/plans/rpc-server.md.
const char* sDevToken = "librepcb-dev-token";

}  // namespace

/*******************************************************************************
 *  FFI Callback
 ******************************************************************************/

bool rpcServerDispatch(void* ctx, const QString* method, const QString* params,
                       QString* response) {
  Q_ASSERT(ctx && method && params && response);
  return static_cast<RpcServer*>(ctx)->dispatchFromWorkerThread(
      *method, *params, *response);
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

RpcServer::RpcServer(GuiApplication& app) noexcept
  : QObject(&app), mApp(app), mServer(nullptr), mShuttingDown(false) {
  mCommands = buildCommandRegistry();

  // Application information to be included in the `initialize` response.
  QJsonObject info;
  info["appVersion"] = Application::getVersion();
  info["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
  info["workspace"] = mApp.getWorkspace().getPath().toStr();
  const QJsonDocument infoDoc(info);

  // Create the server. Note that the token is hardcoded for now (see
  // sDevToken above).
  const QString token = sDevToken;
  const QString protocolVersion = "1";
  const QString infoStr =
      QString::fromUtf8(infoDoc.toJson(QJsonDocument::Compact));
  mServer = rs::ffi_rpc_server_new(&token, &protocolVersion, &infoStr,
                                   &rpcServerDispatch, this);
  if (!mServer) {
    qCritical() << "Failed to create the RPC server.";
    return;
  }

  // Provide the API schema for the `rpc.discover` method. If it is not
  // found, the server still works but does not expose its API.
  const FilePath schemaFp =
      Application::getResourcesDir().getPathTo("rpc/api.json");
  QFile schemaFile(schemaFp.toStr());
  if (schemaFile.open(QIODevice::ReadOnly)) {
    const QString schema = QString::fromUtf8(schemaFile.readAll());
    rs::ffi_rpc_server_set_api_schema(mServer, &schema);
  } else {
    qWarning() << "RPC API schema not found:" << schemaFp.toNative();
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
  mShuttingDown.store(true);

  // Let any in-flight dispatch complete before the members are destroyed.
  // Without this, a command which was marshalled to the GUI thread right
  // before would block on the (already stopped) event loop forever.
  QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);

  if (mServer) {
    rs::ffi_rpc_server_delete(mServer);  // Stops the server & joins threads.
    mServer = nullptr;
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void RpcServer::notifyEvent(const QString& method,
                            const QString& params) noexcept {
  if (mServer) {
    rs::ffi_rpc_server_notify(mServer, &method, &params);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool RpcServer::dispatchFromWorkerThread(const QString& method,
                                         const QString& params,
                                         QString& response) noexcept {
  if (mShuttingDown.load()) {
    // The event loop is already stopped, so marshalling would deadlock.
    writeErrorResponse(response, ServerShuttingDown,
                       "The server is shutting down");
    return false;
  }
  if (QThread::currentThread() == thread()) {
    // Just a safety net, commands are never dispatched from the GUI thread.
    return dispatch(method, params, response);
  }

  // Marshal the command to the GUI thread, blocking until it was executed.
  // Note: This can never deadlock with the GUI thread waiting for this
  // server, since GUI-thread code must never synchronously call into this
  // server (see class documentation).
  bool ok = false;
  if (!QMetaObject::invokeMethod(
          this, [&]() { ok = dispatch(method, params, response); },
          Qt::BlockingQueuedConnection)) {
    writeErrorResponse(response, CommandFailed,
                       "Failed to dispatch the command");
    return false;
  }
  return ok;
}

bool RpcServer::dispatch(const QString& method, const QString& params,
                         QString& response) noexcept {
  try {
    const auto handler = mCommands.constFind(method);
    if (handler == mCommands.constEnd()) {
      writeErrorResponse(response, MethodNotFound,
                         QString("Method not found: %1").arg(method));
      return false;
    }

    // Note: The params were already validated by the Rust server, so
    // parsing them can't fail. We're still defensive to never pass
    // garbage to the command handlers.
    QJsonParseError parseError;
    const QJsonDocument doc =
        QJsonDocument::fromJson(params.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
      writeErrorResponse(response, InvalidParams, "Invalid params");
      return false;
    }

    const RpcResult result = handler.value()(mApp, doc.object());
    if (result.ok) {
      response = QString::fromUtf8(
          QJsonDocument(result.result).toJson(QJsonDocument::Compact));
      return true;
    }
    writeErrorResponse(response, result.errorCode, result.errorMessage);
    return false;
  } catch (const Exception& e) {
    writeErrorResponse(response, CommandFailed,
                       QString("Command failed: %1").arg(e.getMsg()));
    return false;
  } catch (const std::exception& e) {
    writeErrorResponse(response, CommandFailed,
                       QString("Command failed: %1").arg(e.what()));
    return false;
  } catch (...) {
    writeErrorResponse(response, CommandFailed,
                       "Command failed with unknown error");
    return false;
  }
}

void RpcServer::writeErrorResponse(QString& response, int code,
                                   const QString& message) noexcept {
  const QJsonObject error{{"code", code}, {"message", message}};
  response =
      QString::fromUtf8(QJsonDocument(error).toJson(QJsonDocument::Compact));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
