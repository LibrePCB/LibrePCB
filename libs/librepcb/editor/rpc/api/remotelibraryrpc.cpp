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
#include "../rpcrequest.h"
#include "rpcerrors.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace rpc {

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void RemoteLibraryRpc::list(GuiApplication& app, RpcObjectRegistry& registry,
                            RpcRequest& request) {
  Q_UNUSED(registry);

  // Make sure the data is available (fetching it from the configured
  // servers happens asynchronously, see the method documentation). The
  // returned list contains whatever is currently available; streaming the
  // libraries as notifications is not implemented yet.
  auto& model = app.getRemoteLibraries();
  model.ensurePopulated(false);

  QVariantList libraries;
  const std::size_t count = model.row_count();
  for (std::size_t i = 0; i < count; ++i) {
    const auto info = model.row_data(i);
    if (!info) continue;
    QVariantMap library;
    library.insert("uuid", s2q(info->uuid));
    library.insert("name", s2q(info->name));
    library.insert("description", s2q(info->description));
    library.insert("installed_version", s2q(info->installed_version));
    library.insert("online_version", s2q(info->online_version));
    library.insert("outdated", info->outdated);
    libraries.append(library);
  }
  QVariantMap result;
  result.insert("libraries", libraries);
  request.sendSuccess(result);
}

void RemoteLibraryRpc::get(GuiApplication& app, RpcObjectRegistry& registry,
                           RpcRequest& request) {
  Q_UNUSED(app);
  Q_UNUSED(registry);
  Q_UNUSED(request);
  // TODO: Remote libraries are not registered objects yet, so there is no
  // object handle to look up. The handle handling for them needs to be
  // designed first (probably by passing the library UUID instead of a
  // handle).
  throw RpcMethodNotAvailable(
      "Getting details about a remote library is not supported yet.");
}

void RemoteLibraryRpc::install(GuiApplication& app, RpcObjectRegistry& registry,
                               RpcRequest& request) {
  Q_UNUSED(app);
  Q_UNUSED(registry);
  Q_UNUSED(request);
  // TODO: The installation is currently driven by the checked state of the
  // libraries model of the library manager GUI (see
  // `LibrariesModel::applyChanges()`), which cannot be used from here
  // without hijacking the GUI state. The installation machinery needs to be
  // decoupled from the GUI first.
  throw RpcMethodNotAvailable(
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
  throw RpcMethodNotAvailable(
      "Uninstalling remote libraries is not supported through the RPC "
      "interface yet.");
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rpc
}  // namespace editor
}  // namespace librepcb
