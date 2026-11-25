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
#include "designsession.h"

#include <librepcb/core/application.h>
#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/network/networkaccessmanager.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectloader.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/undocommand.h>
#include <librepcb/editor/undostack.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace automation {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DesignSession::DesignSession() noexcept {
}

DesignSession::~DesignSession() noexcept {
  closeProject();
  mWorkspace.reset();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

Workspace& DesignSession::getWorkspace() {
  if (!mWorkspace) {
    throw RuntimeError(__FILE__, __LINE__,
                       "No workspace is open. Call workspace_open first.");
  }
  return *mWorkspace;
}

Project& DesignSession::getProject() {
  if (!mProject) {
    throw RuntimeError(__FILE__, __LINE__,
                       "No project is open. Call project_open first.");
  }
  return *mProject;
}

editor::UndoStack& DesignSession::getUndoStack() {
  getProject();  // can throw
  return *mUndoStack;
}

Schematic& DesignSession::getSchematic(const QJsonValue& nameOrIndex) {
  const QList<Schematic*>& schematics = getProject().getSchematics();
  if (nameOrIndex.isString()) {
    for (Schematic* schematic : schematics) {
      if (*schematic->getName() == nameOrIndex.toString()) {
        return *schematic;
      }
    }
  } else if (Schematic* schematic =
                 schematics.value(nameOrIndex.toInt(0), nullptr)) {
    return *schematic;
  }
  throw RuntimeError(__FILE__, __LINE__,
                     QString("Schematic '%1' not found.")
                         .arg(nameOrIndex.toVariant().toString()));
}

Board& DesignSession::getBoard(const QJsonValue& nameOrIndex) {
  const QList<Board*>& boards = getProject().getBoards();
  if (nameOrIndex.isString()) {
    for (Board* board : boards) {
      if (*board->getName() == nameOrIndex.toString()) {
        return *board;
      }
    }
  } else if (Board* board = boards.value(nameOrIndex.toInt(0), nullptr)) {
    return *board;
  }
  throw RuntimeError(
      __FILE__, __LINE__,
      QString("Board '%1' not found.").arg(nameOrIndex.toVariant().toString()));
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void DesignSession::openWorkspace(const FilePath& fp) {
  mWorkspace.reset();
  if (!Workspace::checkCompatibility(fp)) {
    if (fp.isExistingFile() || (fp.isExistingDir() && (!fp.isEmptyDir()))) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString("'%1' is neither a LibrePCB workspace nor an empty "
                  "directory.")
              .arg(fp.toNative()));
    }
    Workspace::createNewWorkspace(fp);  // can throw
  }
  QString copyFromDir, copyToDir;
  const QString dataDir = Workspace::determineDataDirectory(
      Workspace::findDataDirectories(fp), copyFromDir,
      copyToDir);  // can throw
  if (!copyFromDir.isEmpty()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        "The workspace needs to be upgraded to a newer file format. Open it "
        "once with the LibrePCB application to do so.");
  }
  mWorkspace = std::make_unique<Workspace>(fp, dataDir);  // can throw
  rescanLibraries();  // can throw
}

void DesignSession::rescanLibraries() {
  WorkspaceLibraryDb& db = getWorkspace().getLibraryDb();  // can throw
  QString error;
  QEventLoop loop;
  QObject::connect(&db, &WorkspaceLibraryDb::scanFailed, &loop,
                   [&error](const QString& msg) { error = msg; });
  QObject::connect(&db, &WorkspaceLibraryDb::scanFinished, &loop,
                   &QEventLoop::quit);
  db.startLibraryRescan();
  loop.exec();
  if (!error.isEmpty()) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Failed to scan the libraries: " % error);
  }
}

void DesignSession::startNetworking() noexcept {
  if (!mNetworkAccessManager) {
    mNetworkAccessManager = std::make_unique<NetworkAccessManager>();
  }
}

void DesignSession::openProject(const FilePath& fp, bool readOnly) {
  if (!fp.isExistingFile()) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Project file '%1' does not exist.").arg(fp.toNative()));
  }
  closeProject();
  auto fs = TransactionalFileSystem::open(fp.getParentDir(),
                                          !readOnly);  // can throw
  ProjectLoader loader;
  std::unique_ptr<Project> project =
      loader.open(std::make_unique<TransactionalDirectory>(fs),
                  fp.getFilename());  // can throw
  mFileSystem = fs;
  mProject = std::move(project);
  mUndoStack = std::make_unique<editor::UndoStack>();
}

void DesignSession::createProject(const FilePath& fp, const QString& name) {
  if (fp.getSuffix() != "lpp") {
    throw RuntimeError(__FILE__, __LINE__,
                       "The project file name must end with '.lpp'.");
  }
  const FilePath dir = fp.getParentDir();
  if (dir.isExistingFile() || (dir.isExistingDir() && (!dir.isEmptyDir()))) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("The directory '%1' is not empty.").arg(dir.toNative()));
  }
  closeProject();

  auto fs = TransactionalFileSystem::openRW(dir);  // can throw
  std::unique_ptr<Project> project =
      Project::create(std::make_unique<TransactionalDirectory>(fs),
                      fp.getFilename());  // can throw
  project->setName(ElementName(cleanElementName(name)));  // can throw
  if (mWorkspace) {
    const WorkspaceSettings& settings = mWorkspace->getSettings();
    project->setAuthor(settings.userName.get());
    project->setLocaleOrder(settings.libraryLocaleOrder.get());
    project->setNormOrder(settings.libraryNormOrder.get());
  }
  project->addSchematic(*new Schematic(
      *project, std::make_unique<TransactionalDirectory>(), "main",
      Uuid::createRandom(), ElementName("Main")));  // can throw
  Board* board = new Board(*project, std::make_unique<TransactionalDirectory>(),
                           "default", Uuid::createRandom(),
                           ElementName("default"));  // can throw
  board->addDefaultContent();
  project->addBoard(*board);

  // Same auxiliary files as created by the new project wizard.
  TransactionalDirectory root(fs);
  const FilePath res = Application::getResourcesDir();
  QByteArray readme =
      FileUtils::readFile(res.getPathTo("project/readme_template"));
  readme.replace("{PROJECT_NAME}", name.toUtf8());
  readme.replace("{LICENSE_TEXT}", "No license set.");
  root.write("README.md", readme);  // can throw
  root.write(".gitignore",
             FileUtils::readFile(res.getPathTo("project/gitignore_template")));
  root.write(
      ".gitattributes",
      FileUtils::readFile(res.getPathTo("project/gitattributes_template")));

  project->save();  // can throw
  fs->save();  // can throw
  mFileSystem = fs;
  mProject = std::move(project);
  mUndoStack = std::make_unique<editor::UndoStack>();
}

void DesignSession::closeProject() noexcept {
  mUndoStack.reset();
  mProject.reset();
  mFileSystem.reset();
}

void DesignSession::saveProject() {
  Project& project = getProject();  // can throw
  if (!mFileSystem->isWritable()) {
    throw RuntimeError(__FILE__, __LINE__, "The project was opened read-only.");
  }
  project.save();  // can throw
  mFileSystem->save();  // can throw
  mUndoStack->setClean();
}

void DesignSession::execCmd(editor::UndoCommand* cmd) {
  std::unique_ptr<editor::UndoCommand> cmdGuard(cmd);
  getUndoStack().execCmd(cmdGuard.release());  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace automation
}  // namespace librepcb
