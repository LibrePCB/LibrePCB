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
#include "projectrpc.h"

#include "../../guiapplication.h"
#include "../../project/projecteditor.h"
#include "../../undostack.h"
#include "../rpcobjectregistry.h"
#include "../rpcrequest.h"
#include "rpcerrors.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Local Functions
 ******************************************************************************/

static QVariantMap projectToVariant(RpcObjectRegistry& registry,
                                    ProjectEditor& editor) {
  const Project& project = editor.getProject();
  QVariantMap map;
  map.insert("obj", registry.getHandle(editor));
  map.insert("uuid", project.getUuid().toStr());
  map.insert("name", *project.getName());
  map.insert("author", project.getAuthor());
  map.insert("version", *project.getVersion());
  map.insert("path", project.getFilepath().toStr());
  map.insert("modified", !editor.getUndoStack().isClean());
  return map;
}

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void ProjectRpc::list(GuiApplication& app, RpcObjectRegistry& registry,
                      RpcRequest& request) {
  QVariantList projects;
  for (const auto& editor : app.getProjects()) {
    projects.append(QVariant(projectToVariant(registry, *editor)));
  }
  QVariantMap result;
  result.insert("projects", projects);
  request.sendSuccess(result);
}

void ProjectRpc::open(GuiApplication& app, RpcObjectRegistry& registry,
                      RpcRequest& request) {
  const FilePath fp(request.getArg("path").toString());
  if (!fp.isExistingFile()) {
    throw RpcCommandFailed("The project file does not exist.");
  }
  auto editor = app.openProject(fp, nullptr);
  if (!editor) {
    throw RpcCommandFailed("Could not open the project.");
  }
  request.sendSuccess(projectToVariant(registry, *editor));
}

void ProjectRpc::get(GuiApplication& app, RpcObjectRegistry& registry,
                     RpcRequest& request) {
  Q_UNUSED(app);
  ProjectEditor& editor = registry.getAs<ProjectEditor>(request.getArg("obj"));
  QVariantMap result = projectToVariant(registry, editor);
  const Project& project = editor.getProject();
  QVariantList sheets;
  for (const Schematic* schematic : project.getSchematics()) {
    QVariantMap sheet;
    sheet.insert("uuid", schematic->getUuid().toStr());
    sheet.insert("name", *schematic->getName());
    sheets.append(QVariant(sheet));
  }
  result.insert("sheets", sheets);
  QVariantList boards;
  for (const Board* board : project.getBoards()) {
    QVariantMap item;
    item.insert("uuid", board->getUuid().toStr());
    item.insert("name", *board->getName());
    boards.append(QVariant(item));
  }
  result.insert("boards", boards);
  request.sendSuccess(result);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
