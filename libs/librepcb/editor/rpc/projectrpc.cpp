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

#include "../guiapplication.h"
#include "../project/projecteditor.h"
#include "../undostack.h"
#include "rpcrequest.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/types/uuid.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Local Functions
 ******************************************************************************/
namespace {

/**
 * @brief Serialize a project to a map
 *
 * @param editor  The project editor of the project to serialize.
 *
 * @note The map includes the reserved "obj" key with the object handle
 *       of the project (an opaque value for clients, see the protocol
 *       conventions).
 */
QVariantMap projectToVariant(ProjectEditor& editor) {
  const Project& project = editor.getProject();
  QVariantMap map;
  map.insert("obj", project.getUuid().toStr());
  map.insert("uuid", project.getUuid().toStr());
  map.insert("name", *project.getName());
  map.insert("author", project.getAuthor());
  map.insert("version", *project.getVersion());
  map.insert("path", project.getFilepath().toStr());
  map.insert("modified", !editor.getUndoStack().isClean());
  return map;
}

/**
 * @brief Find the project editor of an open project
 *
 * @param app   The application to look up the project in.
 * @param obj   The project handle (the "obj" key of the request params).
 *
 * @return The requested project editor, or nullptr if there is no such
 *         project open (or the handle is missing/invalid).
 */
ProjectEditor* findProject(GuiApplication& app, RpcRequest& request) noexcept {
  try {
    const Uuid uuid =
        Uuid::fromString(request.getArgs().value("obj").toString());
    for (const auto& editor : app.getProjects()) {
      if (editor->getProject().getUuid() == uuid) {
        return editor.get();
      }
    }
  } catch (const Exception&) {
    // Missing or invalid project handle → considered "not open".
  }
  return nullptr;
}

}  // namespace

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void ProjectRpc::list(GuiApplication& app, RpcRequest& request) {
  QVariantList projects;
  for (const auto& editor : app.getProjects()) {
    projects.append(QVariant(projectToVariant(*editor)));
  }
  QVariantMap result;
  result.insert("projects", projects);
  request.sendSuccess(result);
}

void ProjectRpc::open(GuiApplication& app, RpcRequest& request) {
  const FilePath fp(request.getArgs().value("path").toString());
  if (!fp.isExistingFile()) {
    request.sendError(rs::RpcErrorCode::CommandFailed,
                      "The project file does not exist.");
    return;
  }
  auto editor = app.openProject(fp, nullptr);
  if (!editor) {
    request.sendError(rs::RpcErrorCode::CommandFailed,
                      "Could not open the project.");
    return;
  }
  request.sendSuccess(projectToVariant(*editor));
}

void ProjectRpc::get(GuiApplication& app, RpcRequest& request) {
  ProjectEditor* editor = findProject(app, request);
  if (!editor) {
    request.sendError(rs::RpcErrorCode::InvalidHandle);
    return;
  }
  QVariantMap result = projectToVariant(*editor);
  const Project& project = editor->getProject();
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
