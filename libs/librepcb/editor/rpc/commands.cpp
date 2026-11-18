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
#include "rpcserver.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/types/uuid.h>

#include <QJsonArray>
#include <QJsonObject>

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
 * @brief Serialize a project to a JSON object
 *
 * @param editor  The project editor of the project to serialize.
 */
QJsonObject projectToJson(ProjectEditor& editor) {
  const Project& project = editor.getProject();
  return QJsonObject{
      {"uuid", project.getUuid().toStr()},
      {"name", *project.getName()},
      {"author", project.getAuthor()},
      {"version", *project.getVersion()},
      {"path", project.getFilepath().toStr()},
      {"modified", !editor.getUndoStack().isClean()},
  };
}

/**
 * @brief Serialize the state of a project's undo stack to a JSON object
 *
 * @param stack  The undo stack to serialize.
 */
QJsonObject undoStackToJson(const UndoStack& stack) {
  return QJsonObject{
      {"canUndo", stack.canUndo()},
      {"canRedo", stack.canRedo()},
      {"undoText", stack.getUndoCmdText()},
      {"redoText", stack.getRedoCmdText()},
      {"clean", stack.isClean()},
  };
}

/**
 * @brief Find the project editor of an open project
 *
 * @param app     The application to look up the project in.
 * @param params  The request params, containing the project UUID in the
 *                key "project".
 *
 * @return The requested project editor, or nullptr if there is no such
 *         project open (or the UUID is missing/invalid).
 */
ProjectEditor* findProject(GuiApplication& app,
                           const QJsonObject& params) noexcept {
  try {
    const Uuid uuid = Uuid::fromString(params.value("project").toString());
    for (const auto& editor : app.getProjects()) {
      if (editor->getProject().getUuid() == uuid) {
        return editor.get();
      }
    }
  } catch (const Exception&) {
    // Missing or invalid project parameter → considered "not open".
  }
  return nullptr;
}

}  // namespace

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

QHash<QString, RpcServer::RpcHandler>
    RpcServer::buildCommandRegistry() noexcept {
  QHash<QString, RpcHandler> registry;

  // List all open projects.
  registry.insert("project/list", [](GuiApplication& app, const QJsonObject&) {
    QJsonArray projects;
    for (const auto& editor : app.getProjects()) {
      projects.append(projectToJson(*editor));
    }
    return RpcResult::success(QJsonObject{{"projects", projects}});
  });

  // Get details about one open project.
  registry.insert("project/get",
                  [](GuiApplication& app, const QJsonObject& params) {
                    ProjectEditor* editor = findProject(app, params);
                    if (!editor) {
                      return RpcResult::failure(
                          ProjectNotOpen,
                          QString("Project not open: %1")
                              .arg(params.value("project").toString()));
                    }
                    QJsonObject result = projectToJson(*editor);
                    const Project& project = editor->getProject();
                    QJsonArray sheets;
                    for (const Schematic* schematic : project.getSchematics()) {
                      sheets.append(QJsonObject{
                          {"uuid", schematic->getUuid().toStr()},
                          {"name", *schematic->getName()},
                      });
                    }
                    result["sheets"] = sheets;
                    QJsonArray boards;
                    for (const Board* board : project.getBoards()) {
                      boards.append(QJsonObject{
                          {"uuid", board->getUuid().toStr()},
                          {"name", *board->getName()},
                      });
                    }
                    result["boards"] = boards;
                    return RpcResult::success(result);
                  });

  // Get the state of a project's undo stack.
  registry.insert(
      "undostack/getStatus",
      [](GuiApplication& app, const QJsonObject& params) {
        ProjectEditor* editor = findProject(app, params);
        if (!editor) {
          return RpcResult::failure(
              ProjectNotOpen,
              QString("Project not open: %1")
                  .arg(params.value("project").toString()));
        }
        return RpcResult::success(undoStackToJson(editor->getUndoStack()));
      });

  // Undo the last command of a project's undo stack.
  registry.insert(
      "undostack/undo", [](GuiApplication& app, const QJsonObject& params) {
        ProjectEditor* editor = findProject(app, params);
        if (!editor) {
          return RpcResult::failure(
              ProjectNotOpen,
              QString("Project not open: %1")
                  .arg(params.value("project").toString()));
        }
        editor->getUndoStack().undo();  // Can throw.
        return RpcResult::success(undoStackToJson(editor->getUndoStack()));
      });

  // Redo the last undone command of a project's undo stack.
  registry.insert(
      "undostack/redo", [](GuiApplication& app, const QJsonObject& params) {
        ProjectEditor* editor = findProject(app, params);
        if (!editor) {
          return RpcResult::failure(
              ProjectNotOpen,
              QString("Project not open: %1")
                  .arg(params.value("project").toString()));
        }
        editor->getUndoStack().redo();  // Can throw.
        return RpcResult::success(undoStackToJson(editor->getUndoStack()));
      });

  return registry;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
