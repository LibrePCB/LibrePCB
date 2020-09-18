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

#ifndef LIBREPCB_PROJECTLIBRARYUPDATER_H
#define LIBREPCB_PROJECTLIBRARYUPDATER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/uuid.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class TransactionalFileSystem;

namespace workspace {
class Workspace;
class WorkspaceLibraryDb;
}  // namespace workspace

namespace application {

class ControlPanel;

namespace Ui {
class ProjectLibraryUpdater;
}

/*******************************************************************************
 *  Class ProjectLibraryUpdater
 ******************************************************************************/

/**
 * @brief The ProjectLibraryUpdater class
 *
 * @note This updater is currently an ugly hack with very limited functionality.
 * The whole project library update concept needs to be refactored some time to
 * provide an updater with much more functionality and higher reliability.
 */
class ProjectLibraryUpdater : public QMainWindow {
  Q_OBJECT

public:
  explicit ProjectLibraryUpdater(workspace::Workspace& ws,
                                 const FilePath& project,
                                 ControlPanel& cp) noexcept;
  ~ProjectLibraryUpdater();

private slots:
  void btnUpdateClicked();

private:
  void log(const QString& msg) noexcept;
  QString prettyPath(const FilePath& fp) const noexcept;
  void updateElements(
      std::shared_ptr<TransactionalFileSystem> fs, const QString& type,
      FilePath (workspace::WorkspaceLibraryDb::*getter)(const Uuid&) const);

private:
  workspace::Workspace& mWorkspace;
  FilePath mProjectFilePath;
  ControlPanel& mControlPanel;
  QScopedPointer<Ui::ProjectLibraryUpdater> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb

#endif  // LIBREPCB_PROJECTLIBRARYUPDATER_H
