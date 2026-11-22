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

#include "../../guiapplication.h"
#include "../../project/projecteditor.h"
#include "../../undostack.h"
#include "../rpcrequest.h"
#include "rpcerrors.h"

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

static QVariantMap undoStackToVariant(const UndoStack& stack) {
  QVariantMap map;
  map.insert("canUndo", stack.canUndo());
  map.insert("canRedo", stack.canRedo());
  map.insert("undoText", stack.getUndoCmdText());
  map.insert("redoText", stack.getRedoCmdText());
  map.insert("clean", stack.isClean());
  return map;
}

static ProjectEditor& findProject(GuiApplication& app, RpcRequest& request) {
  if (auto uuid = Uuid::tryFromString(request.getArg("obj").toString())) {
    for (const auto& editor : app.getProjects()) {
      if (editor->getProject().getUuid() == uuid) {
        return *editor;
      }
    }
  }
  throw RpcInvalidHandle();
}

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void UndoStackRpc::get_status(GuiApplication& app, RpcRequest& request) {
  ProjectEditor& editor = findProject(app, request);
  request.sendSuccess(undoStackToVariant(editor.getUndoStack()));
}

void UndoStackRpc::undo(GuiApplication& app, RpcRequest& request) {
  ProjectEditor& editor = findProject(app, request);
  editor.getUndoStack().undo();  // Can throw.
  request.sendSuccess(undoStackToVariant(editor.getUndoStack()));
}

void UndoStackRpc::redo(GuiApplication& app, RpcRequest& request) {
  ProjectEditor& editor = findProject(app, request);
  editor.getUndoStack().redo();  // Can throw.
  request.sendSuccess(undoStackToVariant(editor.getUndoStack()));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
