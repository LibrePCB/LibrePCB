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
#include "projecteditorrpc.h"

#include "../../guiapplication.h"
#include "../../project/projecteditor.h"
#include "../rpcobjectregistry.h"
#include "../rpcrequest.h"
#include "rpcerrors.h"
#include "rpchelpers.h"
#include "rpctypes.h"

#include <librepcb/core/attribute/attribute.h>
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
namespace rpc {

/*******************************************************************************
 *  Helper Functions
 ******************************************************************************/

static QVariantMap summary(const Attribute& obj) {
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
  result.insert("uuid", toVariant(obj.getUuid()));
  result.insert("name", *obj.getName());
  return result;
}

static QVariantMap summary(Board& obj, RpcObjectRegistry& registry) {
  QVariantMap result;
  result.insert("obj", registry.getHandle(obj));
  result.insert("uuid", toVariant(obj.getUuid()));
  result.insert("name", *obj.getName());
  return result;
}

static QVariantMap summary(ProjectEditor& obj, RpcObjectRegistry& registry) {
  const Project& project = obj.getProject();

  QVariantMap result;
  result.insert("obj", registry.getHandle(obj));
  result.insert("name", *project.getName());
  result.insert("path", project.getFilepath().toStr());
  result.insert("dirty", obj.hasUnsavedChanges());
  return result;
}

static QVariantMap details(ProjectEditor& obj, RpcObjectRegistry& registry) {
  const Project& project = obj.getProject();

  QVariantMap result;
  result.insert("uuid", toVariant(project.getUuid()));
  result.insert("name", *project.getName());
  result.insert("path", project.getFilepath().toStr());
  result.insert("dirty", obj.hasUnsavedChanges());
  result.insert("author", project.getAuthor());
  result.insert("version", *project.getVersion());
  result.insert("attributes",
                toVariant(project.getAttributes(),
                          [&](const Attribute& obj) { return summary(obj); }));
  result.insert("locale_order", toVariant(project.getLocaleOrder()));
  result.insert("norm_order", toVariant(project.getNormOrder()));
  result.insert("sheets",
                toVariant(project.getSchematics(), [&](Schematic& obj) {
                  return summary(obj, registry);
                }));
  result.insert("boards", toVariant(project.getBoards(), [&](Board& obj) {
                  return summary(obj, registry);
                }));
  return result;
}

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void ProjectEditorRpc::list(GuiApplication& app, RpcObjectRegistry& registry,
                            RpcRequest& request) {
  QVariantList projects;
  for (const auto& editor : app.getProjects()) {
    projects.append(summary(*editor, registry));
  }
  QVariantMap result;
  result.insert("projects", projects);
  request.send(result);
}

void ProjectEditorRpc::open(GuiApplication& app, RpcObjectRegistry& registry,
                            RpcRequest& request) {
  const FilePath fp(getArg<QString>(request, "path"));
  const auto backupRestoreMode =
      getOptionalArg<BackupRestoreMode>(request, "backup")
          .value_or(BackupRestoreMode::Abort);
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
  request.send(details(*editor, registry));
}

void ProjectEditorRpc::get(GuiApplication& app, RpcObjectRegistry& registry,
                           RpcRequest& request) {
  Q_UNUSED(app);
  auto& editor = registry.getAs<ProjectEditor>(request.getArg("obj"));
  request.send(details(editor, registry));
}

void ProjectEditorRpc::save(GuiApplication& app, RpcObjectRegistry& registry,
                            RpcRequest& request) {
  Q_UNUSED(app);
  auto& editor = registry.getAs<ProjectEditor>(request.getArg("obj"));
  editor.saveProjectNonInteractive();  // can throw
  request.send();
}

void ProjectEditorRpc::close(GuiApplication& app, RpcObjectRegistry& registry,
                             RpcRequest& request) {
  auto& editor = registry.getAs<ProjectEditor>(request.getArg("obj"));
  const auto& projects = app.getProjects();
  for (int i = 0; i < projects.count(); ++i) {
    if (projects.at(i).get() == &editor) {
      app.closeProject(i);
      request.send();
      return;
    }
  }
  throw RpcInternalError();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rpc
}  // namespace editor
}  // namespace librepcb
