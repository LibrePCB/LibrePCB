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
#include "undostackrpc.h"

#include "../guiapplication.h"
#include "../project/projecteditor.h"
#include "../undostack.h"
#include "rpcrequest.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/project/project.h>
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
 * @brief Find the project editor of an open project (by its handle)
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

void UndoStackRpc::get_status(GuiApplication& app, RpcRequest& request) {
  ProjectEditor* editor = findProject(app, request);
  if (!editor) {
    request.sendError(rs::RpcErrorCode::InvalidHandle);
    return;
  }
  request.sendSuccess(undoStackToVariant(editor->getUndoStack()));
}

void UndoStackRpc::undo(GuiApplication& app, RpcRequest& request) {
  ProjectEditor* editor = findProject(app, request);
  if (!editor) {
    request.sendError(rs::RpcErrorCode::InvalidHandle);
    return;
  }
  editor->getUndoStack().undo();  // Can throw.
  request.sendSuccess(undoStackToVariant(editor->getUndoStack()));
}

void UndoStackRpc::redo(GuiApplication& app, RpcRequest& request) {
  ProjectEditor* editor = findProject(app, request);
  if (!editor) {
    request.sendError(rs::RpcErrorCode::InvalidHandle);
    return;
  }
  editor->getUndoStack().redo();  // Can throw.
  request.sendSuccess(undoStackToVariant(editor->getUndoStack()));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
