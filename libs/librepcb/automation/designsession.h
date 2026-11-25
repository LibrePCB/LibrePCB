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

#ifndef LIBREPCB_AUTOMATION_DESIGNSESSION_H
#define LIBREPCB_AUTOMATION_DESIGNSESSION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class NetworkAccessManager;
class Project;
class Schematic;
class TransactionalFileSystem;
class Workspace;

namespace editor {
class UndoCommand;
class UndoStack;
}  // namespace editor

namespace automation {

/*******************************************************************************
 *  Class DesignSession
 ******************************************************************************/

/**
 * @brief The workspace and project which the automation tools operate on
 *
 * At most one workspace and one project are open at a time. Both are opened
 * writable, i.e. their directories are locked until they are closed.
 */
class DesignSession final {
  Q_DECLARE_TR_FUNCTIONS(DesignSession)

public:
  // Constructors / Destructor
  DesignSession(const DesignSession& other) = delete;
  DesignSession() noexcept;
  ~DesignSession() noexcept;

  // Getters
  bool hasWorkspace() const noexcept { return mWorkspace != nullptr; }
  Workspace& getWorkspace();
  bool hasProject() const noexcept { return mProject != nullptr; }
  Project& getProject();
  editor::UndoStack& getUndoStack();
  Schematic& getSchematic(const QJsonValue& nameOrIndex);
  Board& getBoard(const QJsonValue& nameOrIndex);

  // General Methods
  void openWorkspace(const FilePath& fp);
  void rescanLibraries();
  void startNetworking() noexcept;
  void openProject(const FilePath& fp, bool readOnly);
  void createProject(const FilePath& fp, const QString& name);
  void closeProject() noexcept;
  void saveProject();
  void execCmd(editor::UndoCommand* cmd);

  // Operator Overloadings
  DesignSession& operator=(const DesignSession& rhs) = delete;

private:  // Data
  std::unique_ptr<NetworkAccessManager> mNetworkAccessManager;
  std::unique_ptr<Workspace> mWorkspace;
  std::shared_ptr<TransactionalFileSystem> mFileSystem;
  std::unique_ptr<Project> mProject;
  std::unique_ptr<editor::UndoStack> mUndoStack;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace automation
}  // namespace librepcb

#endif
