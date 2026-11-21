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
#include "../guiapplication.h"
#include "../project/projecteditor.h"
#include "../undostack.h"
#include "rpcrequest.h"
#include "rpcserver.h"

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
 * @brief Serialize the state of a project's undo stack to a map
 *
 * @param stack  The undo stack to serialize.
 */
QVariantMap undoStackToVariant(const UndoStack& stack) {
  QVariantMap map;
  map.insert("canUndo", stack.canUndo());
  map.insert("canRedo", stack.canRedo());
  map.insert("undoText", stack.getUndoCmdText());
  map.insert("redoText", stack.getRedoCmdText());
  map.insert("clean", stack.isClean());
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
ProjectEditor* findProject(RpcRequest& request) noexcept {
  try {
    const Uuid uuid =
        Uuid::fromString(request.getArgs().value("obj").toString());
    for (const auto& editor : request.getApp().getProjects()) {
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

QHash<QString, RpcServer::RpcHandler>
    RpcServer::buildCommandRegistry() noexcept {
  using Request = RpcRequest;
  QHash<QString, RpcHandler> registry;

  // List all open projects.
  registry.insert("Project.list", [](Request& request) {
    QVariantList projects;
    for (const auto& editor : request.getApp().getProjects()) {
      projects.append(QVariant(projectToVariant(*editor)));
    }
    QVariantMap result;
    result.insert("projects", projects);
    request.sendSuccess(result);
  });

  // Get details about an open project.
  registry.insert("Project.get", [](Request& request) {
    ProjectEditor* editor = findProject(request);
    if (!editor) {
      request.sendError(rs::RpcErrorCode::ProjectNotOpen, "Project not open");
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
  });

  // Get the state of a project's undo stack.
  registry.insert("UndoStack.get_status", [](Request& request) {
    ProjectEditor* editor = findProject(request);
    if (!editor) {
      request.sendError(rs::RpcErrorCode::ProjectNotOpen, "Project not open");
      return;
    }
    request.sendSuccess(undoStackToVariant(editor->getUndoStack()));
  });

  // Undo the last command of a project's undo stack.
  registry.insert("UndoStack.undo", [](Request& request) {
    ProjectEditor* editor = findProject(request);
    if (!editor) {
      request.sendError(rs::RpcErrorCode::ProjectNotOpen, "Project not open");
      return;
    }
    editor->getUndoStack().undo();  // Can throw.
    request.sendSuccess(undoStackToVariant(editor->getUndoStack()));
  });

  // Redo the last undone command of a project's undo stack.
  registry.insert("UndoStack.redo", [](Request& request) {
    ProjectEditor* editor = findProject(request);
    if (!editor) {
      request.sendError(rs::RpcErrorCode::ProjectNotOpen, "Project not open");
      return;
    }
    editor->getUndoStack().redo();  // Can throw.
    request.sendSuccess(undoStackToVariant(editor->getUndoStack()));
  });

  return registry;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
