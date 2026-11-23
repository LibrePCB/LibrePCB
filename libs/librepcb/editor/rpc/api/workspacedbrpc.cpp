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
#include "../rpcrequest.h"
#include "rpcerrors.h"

#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

using namespace rpc;

/*******************************************************************************
 *  Helper Functions
 ******************************************************************************/

static QVariantMap details(WorkspaceLibraryDb& obj) {
  Q_UNUSED(obj);
  QVariantMap result;
  return result;
}

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void WorkspaceDbRpc::wait(GuiApplication& app, RpcObjectRegistry& registry,
                          RpcRequest& request) {
  Q_UNUSED(registry);
  WorkspaceLibraryDb& db = app.getWorkspace().getLibraryDb();
  if (!db.isScanInProgress()) {
    request.sendSuccess(details(db));
    return;
  }

  request.sendProgress(db.getScanProgressPercent());
  QObject::connect(&db, &WorkspaceLibraryDb::scanProgressUpdate, &request,
                   &RpcRequest::sendProgress);
  QObject::connect(&db, &WorkspaceLibraryDb::scanFinished, &request,
                   [&db, &request]() { request.sendSuccess(details(db)); });
  QObject::connect(&request, &RpcRequest::cancelled, &request,
                   [&request]() { request.sendError(RpcCancelled()); });
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
