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
#include "../rpcobjectregistry.h"
#include "../rpcrequest.h"
#include "rpcerrors.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace rpc {

/*******************************************************************************
 *  Helper Functions
 ******************************************************************************/

static UndoStack& getObj(RpcObjectRegistry& registry, RpcRequest& request) {
  ProjectEditor& editor = registry.getAs<ProjectEditor>(request.getArg("obj"));
  return editor.getUndoStack();
}

static QVariantMap details(UndoStack& obj) {
  QVariantMap result;
  result.insert("id", obj.getUniqueStateId());
  result.insert("clean", obj.isClean());
  result.insert("prev", obj.canUndo() ? obj.getUndoCmdText() : QVariant());
  result.insert("next", obj.canRedo() ? obj.getRedoCmdText() : QVariant());
  return result;
}

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void UndoStackRpc::get(GuiApplication& app, RpcObjectRegistry& registry,
                       RpcRequest& request) {
  Q_UNUSED(app);
  auto& obj = getObj(registry, request);
  request.sendSuccess(details(obj));
}

void UndoStackRpc::undo(GuiApplication& app, RpcObjectRegistry& registry,
                        RpcRequest& request) {
  Q_UNUSED(app);
  auto& obj = getObj(registry, request);
  if (!obj.canUndo()) {
    throw RpcMethodNotAvailable();
  }
  obj.undo();  // Can throw.
  request.sendSuccess(details(obj));
}

void UndoStackRpc::redo(GuiApplication& app, RpcObjectRegistry& registry,
                        RpcRequest& request) {
  Q_UNUSED(app);
  auto& obj = getObj(registry, request);
  if (!obj.canRedo()) {
    throw RpcMethodNotAvailable();
  }
  obj.redo();  // Can throw.
  request.sendSuccess(details(obj));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rpc
}  // namespace editor
}  // namespace librepcb
