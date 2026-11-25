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
#include "../../project/board/boardeditor.h"
#include "../../project/cmd/cmdprojectedit.h"
#include "../../project/newprojectwizard/newprojectwizard.h"
#include "../../project/projecteditor.h"
#include "../../project/schematic/schematiceditor.h"
#include "../../undostack.h"
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
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

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
  return {
      {"name", *obj.getKey()},
      {"type", obj.getType().getName()},
      {"value", obj.getValue()},
      {"unit", obj.getUnit() ? obj.getUnit()->getName() : QVariant()},
  };
}

static QVariantMap summary(SchematicEditor& obj, RpcObjectRegistry& registry) {
  Schematic& schematic = obj.getSchematic();
  return {
      {"obj", registry.getHandle(obj)},
      {"uuid", toVariant(schematic.getUuid())},
      {"name", *schematic.getName()},
  };
}

static QVariantMap summary(BoardEditor& obj, RpcObjectRegistry& registry) {
  Board& board = obj.getBoard();
  return {
      {"obj", registry.getHandle(obj)},
      {"uuid", toVariant(board.getUuid())},
      {"name", *board.getName()},
  };
}

static QVariantMap summary(ProjectEditor& obj, RpcObjectRegistry& registry) {
  const Project& project = obj.getProject();
  return {
      {"obj", registry.getHandle(obj)},
      {"name", *project.getName()},
      {"path", project.getFilepath().toStr()},
      {"dirty", obj.hasUnsavedChanges()},
  };
}

static QVariantMap details(ProjectEditor& obj, RpcObjectRegistry& registry) {
  const Project& project = obj.getProject();
  const auto attributes =
      toVariant(project.getAttributes(),
                [&](const Attribute& obj) { return summary(obj); });
  const auto sheets = toVariant(
      obj.getSchematics(), [&](const std::shared_ptr<SchematicEditor>& obj) {
        return summary(*obj, registry);
      });
  const auto boards =
      toVariant(obj.getBoards(), [&](const std::shared_ptr<BoardEditor>& obj) {
        return summary(*obj, registry);
      });
  return {
      {"uuid", toVariant(project.getUuid())},
      {"name", *project.getName()},
      {"path", project.getFilepath().toStr()},
      {"dirty", obj.hasUnsavedChanges()},
      {"author", project.getAuthor()},
      {"version", *project.getVersion()},
      {"attributes", attributes},
      {"locale_order", toVariant(project.getLocaleOrder())},
      {"norm_order", toVariant(project.getNormOrder())},
      {"sheets", sheets},
      {"boards", boards},
  };
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
  request.send({{"projects", projects}});
}

void ProjectEditorRpc::create(GuiApplication& app, RpcObjectRegistry& registry,
                              RpcRequest& request) {
  Q_UNUSED(registry);
  const FilePath fp(getArg<QString>(request, "path"));
  const auto name = getOptionalArg<ElementName>(request, "name")
                        .value_or(ElementName(fp.getCompleteBasename()));
  const auto license =
      getOptionalArg<QString>(request, "license").value_or(QString());
  const auto& ws = app.getWorkspace().getSettings();
  auto project = NewProjectWizard::createProject(
      ws, fp, name, ws.userName.get(), license, nullptr, nullptr);
  project.reset();  // Release lock.
  auto editor = app.openProjectNonInteractive(
      fp, &TransactionalFileSystem::RestoreMode::abort);
  if (!editor) throw RpcInternalError();
  request.send(details(*editor, registry));
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

void ProjectEditorRpc::set(GuiApplication& app, RpcObjectRegistry& registry,
                           RpcRequest& request) {
  Q_UNUSED(app);
  auto& editor = registry.getAs<ProjectEditor>(request.getArg("obj"));
  auto cmd = std::make_unique<CmdProjectEdit>(editor.getProject());
  if (auto v = getOptionalArg<ElementName>(request, "name")) {
    cmd->setName(*v);
  }
  if (auto v = getOptionalArg<QString>(request, "author")) {
    cmd->setAuthor(*v);
  }
  if (auto v = getOptionalArg<FileProofName>(request, "version")) {
    cmd->setVersion(*v);
  }
  if (auto v = getOptionalArg<AttributeList>(request, "attributes")) {
    cmd->setAttributes(*v);
  }
  if (auto v = getOptionalArg<QStringList>(request, "locale_order")) {
    cmd->setLocaleOrder(*v);  // TODO: Some more validation?
  }
  if (auto v = getOptionalArg<QStringList>(request, "norm_order")) {
    cmd->setNormOrder(*v);  // TODO: Some more validation?
  }
  editor.getUndoStack().execCmd(cmd.release());
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
