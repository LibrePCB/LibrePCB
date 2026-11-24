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
#include "remotelibraryrpc.h"

#include "../../guiapplication.h"
#include "../../library/librariesmodel.h"
#include "../../utils/slinthelpers.h"
#include "../rpcnotification.h"
#include "../rpcrequest.h"
#include "rpcerrors.h"
#include "rpchelpers.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace rpc {

/*******************************************************************************
 *  Helper Functions
 ******************************************************************************/

static ui::LibraryInfoData getObj(const RpcRequest& request,
                                  LibrariesModel& model) {
  const auto uuid = toUuid(request.getArg("obj"));
  for (std::size_t i = 0; i < model.row_count(); ++i) {
    if (auto item = model.row_data(i)) {
      if (item->uuid == uuid.toStr()) {
        return *item;
      }
    }
  }
  throw RpcInvalidHandle();
}

static QVariantMap summary(const ui::LibraryInfoData& obj) {
  QVariantMap result;
  result.insert("uuid", s2q(obj.uuid));
  result.insert("name", s2q(obj.name));
  result.insert("description", s2q(obj.description));
  result.insert("installed_version", s2q(obj.installed_version));
  result.insert("online_version", s2q(obj.online_version));
  result.insert("outdated", obj.outdated);
  return result;
}

static QVariantMap details(const ui::LibraryInfoData& obj) {
  QVariantMap result = summary(obj);
  // TODO: Add more properties.
  return result;
}

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void RemoteLibraryRpc::list(GuiApplication& app, RpcObjectRegistry& registry,
                            RpcRequest& request) {
  Q_UNUSED(registry);

  auto& model = app.getRemoteLibraries();
  model.ensurePopulated(false);

  auto createResponse = [&]() {
    QVariantMap result;
    result.insert("libraries",
                  modelToVariant(model, [](const ui::LibraryInfoData& obj) {
                    return summary(obj);
                  }));
    return result;
  };

  if (!model.isOnlineCheckInProgress()) {
    // Libraries are available -> synchronous response.
    request.send(createResponse());
  } else {
    // Libraries are not available yet -> asynchronous response.
    // NOTE: The partial response streaming (through notifications) is not
    // implemented yet. Also no real progress report yet. Just an immediate
    // notification to show some activity, and the full response when ready.
    // In addition, cancelling the remote library fetching currently cannot
    // be distinguished from a completed fetching.
    request.makeAsync(model);
    request.send(RpcNotification());
    QObject::connect(
        &model, &LibrariesModel::allOnlineLibrariesFetched, &request,
        [&request, createResponse]() { request.send(createResponse()); });
  }
}

void RemoteLibraryRpc::install(GuiApplication& app, RpcObjectRegistry& registry,
                               RpcRequest& request) {
  Q_UNUSED(app);
  Q_UNUSED(registry);
  Q_UNUSED(request);

  const auto uuids = toList<Uuid>(request.getArg("libraries"));

  throw RpcUnexpectedState(
      "Installing remote libraries is not supported through the RPC "
      "interface yet.");
}

void RemoteLibraryRpc::uninstall(GuiApplication& app,
                                 RpcObjectRegistry& registry,
                                 RpcRequest& request) {
  Q_UNUSED(app);
  Q_UNUSED(registry);
  Q_UNUSED(request);
  // TODO: See install().
  throw RpcUnexpectedState(
      "Uninstalling remote libraries is not supported through the RPC "
      "interface yet.");
}

void RemoteLibraryRpc::get(GuiApplication& app, RpcObjectRegistry& registry,
                           RpcRequest& request) {
  Q_UNUSED(app);
  Q_UNUSED(registry);

  auto& model = app.getRemoteLibraries();
  const auto obj = getObj(request, model);
  request.send(details(obj));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rpc
}  // namespace editor
}  // namespace librepcb
