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
#include "rpctypes.h"

#include <librepcb/core/attribute/attributetype.h>
#include <librepcb/core/attribute/attributeunit.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

// The generated RPC types live in their own namespace; open it here to
// avoid qualified names in the command implementations.
using namespace rpc;

/*******************************************************************************
 *  Local Functions
 ******************************************************************************/

static QVariantMap summary(ProjectEditor& obj, RpcObjectRegistry& registry) {
  const Project& project = obj.getProject();

  QVariantMap result;
  result.insert("obj", registry.getHandle(obj));
  result.insert("name", *project.getName());
  result.insert("path", project.getFilepath().toStr());
  result.insert("dirty", !obj.getUndoStack().isClean());
  return result;
}

static QVariantMap summary(Attribute& obj) {
  QVariantMap result;
  result.insert("name", *obj.getKey());
  result.insert("type", obj.getType().getName());
  result.insert("value", obj.getValue());
  result.insert("unit", obj.getUnit() ? obj.getUnit()->getName() : QVariant());
  return result;
}

static QVariantMap summary(Schematic& obj, RpcObjectRegistry& registry) {
  QVariantMap result;
  result.insert("obj", registry.getHandle(obj));
  result.insert("uuid", obj.getUuid().toStr());
  result.insert("name", *obj.getName());
  return result;
}

static QVariantMap summary(Board& obj, RpcObjectRegistry& registry) {
  QVariantMap result;
  result.insert("obj", registry.getHandle(obj));
  result.insert("uuid", obj.getUuid().toStr());
  result.insert("name", *obj.getName());
  return result;
}

template <typename T>
static QVariantList summaryList(const T& list, RpcObjectRegistry& registry) {
  QVariantList result;
  for (auto obj : list) {
    result.append(summary(*obj, registry));
  }
  return result;
}

static QVariantMap details(ProjectEditor& editor, RpcObjectRegistry& registry) {
  const Project& project = editor.getProject();

  QVariantMap result = summary(editor, registry);
  result.insert("author", project.getAuthor());
  result.insert("version", *project.getVersion());
  result.insert("attributes", summaryList(project.getAttributes(), registry));
  result.insert("locale_order", project.getLocaleOrder());
  result.insert("norm_order", project.getNormOrder());
  result.insert("sheets", summaryList(project.getSchematics(), registry));
  result.insert("boards", summaryList(project.getBoards(), registry));
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
  const auto backupRestoreMode = request.getArg<BackupRestoreMode>("backup");
  auto askForRestoringBackup = [backupRestoreMode](const FilePath&) {
    switch (backupRestoreMode) {
      case BackupRestoreMode::Restore:
        return true;
      case BackupRestoreMode::Discard:
        return false;
      case BackupRestoreMode::Abort:
        throw RpcCommandFailed(
            "The project directory contains an auto-save backup (possibly from "
            "a previous application crash while having unsaved changes). Pass "
            "the 'backup' argument to specify how to proceed in this case.");
    }
  };
  auto editor = app.openProjectNonInteractive(fp, askForRestoringBackup);
  if (!editor) throw RpcInternalError();
  request.sendSuccess(details(*editor, registry));
}

void ProjectRpc::get(GuiApplication& app, RpcObjectRegistry& registry,
                     RpcRequest& request) {
  Q_UNUSED(app);
  auto& editor = registry.getAs<ProjectEditor>(request.getArg("obj"));
  request.sendSuccess(details(editor, registry));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
