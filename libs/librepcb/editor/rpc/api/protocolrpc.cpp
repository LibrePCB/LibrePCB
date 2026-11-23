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
#include "protocolrpc.h"

#include "../../guiapplication.h"
#include "../../notification.h"
#include "../../notificationsmodel.h"
#include "../rpcrequest.h"
#include "rpcerrors.h"

#include <librepcb/core/application.h>
#include <librepcb/core/types/uuid.h>
#include <librepcb/core/workspace/workspace.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

using namespace rpc;

/*******************************************************************************
 *  Helper Functions
 ******************************************************************************/

/// The tokens which have been issued by this application instance
/// (see the RPC server documentation).
///
/// TODO: This is currently a per-process static set without any expiry
/// handling (development only). Refactor into a proper class, see the
/// plan section 9.
static QSet<QString>& getAuthenticatedTokens() noexcept {
  static QSet<QString> tokens;
  return tokens;
}

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void ProtocolRpc::initialize(GuiApplication& app, RpcObjectRegistry& registry,
                             RpcRequest& request) {
  Q_UNUSED(app);
  Q_UNUSED(registry);

  const QString clientName =
      request.getArg("name").toString().simplified().left(32);
  const QVariant token = request.getArg("token");

  auto createResponse = [clientName, &app](QString token) {
    static const Uuid session = Uuid::createRandom();

    if (token.isEmpty()) {
      token = Uuid::createRandom().toStr();
      getAuthenticatedTokens().insert(token);
      qInfo() << "RPC client" << clientName << "connected.";
    }

    QVariantMap result;
    result.insert("format", "1.0");
    result.insert("server", "librepcb");
    result.insert("version", Application::getVersion());
    result.insert("session", session.toStr());
    result.insert("workspace", app.getWorkspace().getPath().toStr());
    result.insert("token", token);
    return result;
  };

  if (!token.isNull()) {
    const bool authenticated =
        getAuthenticatedTokens().contains(token.toString());
    if (authenticated) {
      request.sendSuccess(createResponse(token.toString()));
    } else {
      request.sendError(rpc::RpcAccessDenied());
    }
  } else {
    auto notification = std::make_shared<Notification>(
        ui::NotificationType::Warning, tr("RPC Connection Request"),
        tr("An external application, which identifies itself as '%1', wants to "
           "connect through the RPC interface to this process for the first "
           "time. Only accept this if you trust that application and the "
           "request was triggered by yourself.")
            .arg(clientName),
        tr("Accept Connection"), QString(), true);
    QObject::connect(notification.get(), &Notification::buttonClicked, &request,
                     [&request, createResponse]() {
                       request.sendSuccess(createResponse(QString()));
                     });
    QObject::connect(
        notification.get(), &Notification::destroyed, &request,
        [&request]() { request.sendError(rpc::RpcAccessDenied()); });
    QObject::connect(&request, &RpcRequest::cancelled, notification.get(),
                     &Notification::dismiss);
    QObject::connect(&request, &RpcRequest::destroyed, notification.get(),
                     &Notification::dismiss);
    app.getNotifications().push(notification);
  }
}

void ProtocolRpc::ping(GuiApplication& app, RpcObjectRegistry& registry,
                       RpcRequest& request) {
  Q_UNUSED(app);
  Q_UNUSED(registry);
  request.sendSuccess();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
