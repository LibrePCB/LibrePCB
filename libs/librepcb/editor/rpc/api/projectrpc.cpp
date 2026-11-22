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

static QVariantMap summary(ProjectEditor& editor, RpcObjectRegistry& registry) {
  const Project& project = editor.getProject();

  QVariantMap result;
  result.insert("obj", registry.getHandle(editor));
  result.insert("name", *project.getName());
  result.insert("path", project.getFilepath().toStr());
  result.insert("dirty", !editor.getUndoStack().isClean());
  return result;
}

static QVariantMap details(ProjectEditor& editor, RpcObjectRegistry& registry) {
  const Project& project = editor.getProject();

  QVariantMap result = summary(editor, registry);
  result.insert("author", project.getAuthor());
  result.insert("version", *project.getVersion());
  {
    QVariantList list;
    for (const Schematic* schematic : project.getSchematics()) {
      QVariantMap sheet;
      sheet.insert("uuid", schematic->getUuid().toStr());
      sheet.insert("name", *schematic->getName());
      list.append(sheet);
    }
    result.insert("sheets", list);
  }
  {
    QVariantList list;
    for (const Board* board : project.getBoards()) {
      QVariantMap item;
      item.insert("uuid", board->getUuid().toStr());
      item.insert("name", *board->getName());
      list.append(item);
    }
    result.insert("boards", list);
  }
  return result;
}

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void ProjectRpc::list(GuiApplication& app, RpcObjectRegistry& registry,
                      RpcRequest& request) {
  QVariantList projects;
  for (const auto& editor : app.getProjects()) {
    projects.append(summary(*editor, registry));
  }
  QVariantMap result;
  result.insert("projects", projects);
  request.sendSuccess(result);
}

void ProjectRpc::open(GuiApplication& app, RpcObjectRegistry& registry,
                      RpcRequest& request) {
  const FilePath fp(request.getArg("path").toString());
  auto askForRestoringBackup = [](const FilePath&) {
    return false;
  };

  auto editor = app.openProjectNonInteractive(fp, askForRestoringBackup);
  if (!editor) {
    throw RpcInternalError();
  }
  request.sendSuccess(details(*editor, registry));
}

void ProjectRpc::get(GuiApplication& app, RpcObjectRegistry& registry,
                     RpcRequest& request) {
  Q_UNUSED(app);
  ProjectEditor& editor = registry.getAs<ProjectEditor>(request.getArg("obj"));
  request.sendSuccess(details(editor, registry));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
