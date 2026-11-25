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
#include "rpchelpers.h"

#include <librepcb/core/sqlitedatabase.h>
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

void WorkspaceDbRpc::search(GuiApplication& app, RpcObjectRegistry& registry,
                            RpcRequest& request) {
  Q_UNUSED(app);
  Q_UNUSED(registry);
  Q_UNUSED(request);
  throw RpcCommandFailed();
}

void WorkspaceDbRpc::query_sql(GuiApplication& app, RpcObjectRegistry& registry,
                               RpcRequest& request) {
  Q_UNUSED(registry);
  const QString queryStr = getArg<QString>(request, "query");

  // IMPORTANT: Open the database in read-only mode!
  SQLiteDatabase db(app.getWorkspace().getLibraryDb().getFilePath(), true);
  QSqlQuery query = db.prepareQuery(queryStr);
  query.setForwardOnly(true);
  db.exec(query);

  QSqlRecord rec = query.record();
  const int columns = rec.count();

  QVariantList rows;
  while (query.next()) {
    QVariantMap values;
    for (int i = 0; i < columns; ++i) {
      values.insert(rec.fieldName(i), query.value(i));
    }
    rows.append(values);
  }

  request.send({{"rows", rows}});
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rpc
}  // namespace editor
}  // namespace librepcb
