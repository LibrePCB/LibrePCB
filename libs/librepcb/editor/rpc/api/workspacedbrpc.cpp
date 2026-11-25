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
#include "workspacedbrpc.h"

#include "../../guiapplication.h"
#include "../rpcnotification.h"
#include "../rpcrequest.h"
#include "rpcerrors.h"

#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace rpc {

/*******************************************************************************
 *  Helper Functions
 ******************************************************************************/

static QVariantMap details(WorkspaceLibraryDb& obj) {
  Q_UNUSED(obj);
  return {};
}

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void WorkspaceDbRpc::wait(GuiApplication& app, RpcObjectRegistry& registry,
                          RpcRequest& request) {
  Q_UNUSED(registry);
  WorkspaceLibraryDb& db = app.getWorkspace().getLibraryDb();
  if (!db.isScanInProgress()) {
    request.send(details(db));
    return;
  }

  request.makeAsync(db);
  request.send(RpcNotification().progress(db.getScanProgressPercent()));
  QObject::connect(&db, &WorkspaceLibraryDb::scanProgressUpdate, &request,
                   [&request](int percent) {
                     request.send(RpcNotification().progress(percent));
                   });
  QObject::connect(&db, &WorkspaceLibraryDb::scanFinished, &request,
                   [&db, &request]() { request.send(details(db)); });
  QObject::connect(&request, &RpcRequest::cancelled, &request,
                   [&request]() { request.send(RpcCancelled()); });
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rpc
}  // namespace editor
}  // namespace librepcb
