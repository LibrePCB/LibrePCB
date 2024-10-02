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
#include "projectsmodel.h"
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/editor/dialogs/directorylockhandlerdialog.h>
#include "../apptoolbox.h"
#include <librepcb/core/project/projectloader.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/utils/scopeguard.h>
#include <QtCore>
#include "projecteditor.h"
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectsModel::ProjectsModel(QObject* parent) noexcept : QObject(parent) {
}

ProjectsModel::~ProjectsModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ProjectsModel::openProject(const FilePath& fp) {
  const QString uniqueFp = fp.toUnique().toStr();
  if (mEditors.contains(uniqueFp)) return;

  // Opening the project can take some time, use wait cursor to provide
  // immediate UI feedback.
  QApplication::setOverrideCursor(Qt::WaitCursor);
  auto cursorScopeGuard = scopeGuard([this]() { QApplication::restoreOverrideCursor(); });

  // Open file system.
  std::shared_ptr<TransactionalFileSystem> fs;
  QString projectFileName = fp.getFilename();
  if (fp.getSuffix() == "lppz") {
    fs = TransactionalFileSystem::openRO(
        FilePath::getRandomTempPath(),
        &TransactionalFileSystem::RestoreMode::no);
    fs->removeDirRecursively();  // 1) Get a clean initial state.
    fs->loadFromZip(fp);  // 2) Load files from ZIP.
    foreach (const QString& fn, fs->getFiles()) {
      if (fn.endsWith(".lpp")) {
        projectFileName = fn;
      }
    }
  } else {
    fs = TransactionalFileSystem::openRW(
        fp.getParentDir(), &askForRestoringBackup,
        DirectoryLockHandlerDialog::createDirectoryLockCallback());
  }

  // Open project.
  ProjectLoader loader;
  std::unique_ptr<Project> project =
      loader.open(std::unique_ptr<TransactionalDirectory>(
                      new TransactionalDirectory(fs)),
                  projectFileName);  // can throw

  // Open editor.
  auto editor = std::make_shared<ProjectEditor>(std::move(project), this);

  // Keep handle.
  mEditors.insert(uniqueFp, editor);
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t ProjectsModel::row_count() const {
  return mItems.size();
}

std::optional<ui::Project> ProjectsModel::row_data(std::size_t i) const {
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool ProjectsModel::askForRestoringBackup(const FilePath& dir) {
  Q_UNUSED(dir);
  QMessageBox::StandardButton btn = QMessageBox::question(
      nullptr, tr("Restore autosave backup?"),
      tr("It seems that the application crashed the last time you opened this "
         "project. Do you want to restore the last autosave backup?"),
      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
      QMessageBox::Cancel);
  switch (btn) {
    case QMessageBox::Yes:
      return true;
    case QMessageBox::No:
      return false;
    default:
      throw UserCanceled(__FILE__, __LINE__);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
