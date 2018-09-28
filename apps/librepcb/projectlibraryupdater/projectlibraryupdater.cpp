/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "projectlibraryupdater.h"

#include "../controlpanel/controlpanel.h"
#include "ui_projectlibraryupdater.h"

#include <librepcb/common/fileio/directorylock.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/library/dev/device.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/project.h>
#include <librepcb/projecteditor/projecteditor.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace application {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectLibraryUpdater::ProjectLibraryUpdater(workspace::Workspace& ws,
                                             const FilePath&       project,
                                             ControlPanel&         cp) noexcept
  : QMainWindow(nullptr),
    mWorkspace(ws),
    mProjectFilePath(project),
    mControlPanel(cp),
    mUi(new Ui::ProjectLibraryUpdater) {
  mUi->setupUi(this);
  mUi->btnUpdate->setText(
      mUi->btnUpdate->text().arg(mProjectFilePath.getBasename()));
  connect(mUi->btnUpdate, &QPushButton::clicked, this,
          &ProjectLibraryUpdater::btnUpdateClicked);
}

ProjectLibraryUpdater::~ProjectLibraryUpdater() {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ProjectLibraryUpdater::btnUpdateClicked() {
  setEnabled(false);
  mUi->log->clear();

  // close project if it is currently open
  bool                            abort = false;
  project::editor::ProjectEditor* editor =
      mControlPanel.getOpenProject(mProjectFilePath);
  if (editor) {
    log(tr("Ask to close project (confirm message box!)"));
    bool close = editor->closeAndDestroy(true, this);
    if (close) {
      delete editor;  // delete editor to make sure the lock is released
                      // immediately
    } else {
      abort = true;
      log(tr("Abort."));
    }
  }

  if (!abort) {
    try {
      // lock project
      log(QString(tr("Lock project %1...")).arg(prettyPath(mProjectFilePath)));
      DirectoryLock lock(mProjectFilePath.getParentDir());
      lock.tryLock();

      // backup the whole library directory
      FilePath libDir = mProjectFilePath.getParentDir().getPathTo("library");
      FilePath libDirBackup =
          mProjectFilePath.getParentDir().getPathTo("library~");
      log(tr("Create backup of library directory..."));
      FileUtils::removeDirRecursively(libDirBackup);
      FileUtils::copyDirRecursively(libDir, libDirBackup);

      // update all elements
      updateElements("cmp", &workspace::WorkspaceLibraryDb::getLatestComponent);
      updateElements("dev", &workspace::WorkspaceLibraryDb::getLatestDevice);
      updateElements("pkg", &workspace::WorkspaceLibraryDb::getLatestPackage);
      updateElements("sym", &workspace::WorkspaceLibraryDb::getLatestSymbol);

      // release lock to allow opening the project
      lock.unlock();

      // check whether project can still be opened of if we broke something
      try {
        log(QString(tr("Open project %1..."))
                .arg(prettyPath(mProjectFilePath)));
        project::Project prj(mProjectFilePath, false, false);
        log(QString(tr("Save project %1..."))
                .arg(prettyPath(mProjectFilePath)));
        prj.save(true);  // force updating library elements file format
      } catch (const Exception& e) {
        // something is broken -> restore backup
        log(QString(tr("ERROR: %1")).arg(e.getMsg()));
        log(tr("Restore backup..."));
        FileUtils::removeDirRecursively(libDir);
        FileUtils::move(libDirBackup, libDir);
        log(tr("Successfully restored backup. No changes were made at all."));
        throw RuntimeError(__FILE__, __LINE__,
                           tr("Failed to update library elements! Probably "
                              "there were breaking "
                              "changes in some library elements."));
      }
      log(tr("Remove backup..."));
      FileUtils::removeDirRecursively(libDirBackup);
      log(tr("[SUCCESS] All library elements updated."));
    } catch (const Exception& e) {
      log(QString(tr("[ERROR] %1")).arg(e.getMsg()));
    }

    // re-open project if it was previously open
    if (editor) {
      mControlPanel.openProject(mProjectFilePath);
      // bring this window to front again (with some delay to make it working
      // properly)
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
      QTimer::singleShot(500, this, &QMainWindow::raise);
      QTimer::singleShot(500, this, &QMainWindow::activateWindow);
#else
      QTimer::singleShot(500, this, SLOT(raise()));
      // QTimer::singleShot(500, this, SLOT(activateWindow()));
#endif
    }
  }

  setEnabled(true);
}

void ProjectLibraryUpdater::log(const QString& msg) noexcept {
  mUi->log->addItem(msg);
  mUi->log->setCurrentRow(mUi->log->count() - 1);
  qApp->processEvents();
}

QString ProjectLibraryUpdater::prettyPath(const FilePath& fp) const noexcept {
  return fp.toRelative(mProjectFilePath.getParentDir());
}

void ProjectLibraryUpdater::updateElements(
    const QString& type,
    FilePath (workspace::WorkspaceLibraryDb::*getter)(const Uuid&) const) {
  FilePath dir =
      mProjectFilePath.getParentDir().getPathTo("library").getPathTo(type);
  QDir::Filters filters = QDir::Dirs | QDir::NoDotAndDotDot;
  foreach (const QFileInfo& info, QDir(dir.toStr()).entryInfoList(filters)) {
    tl::optional<Uuid> uuid = Uuid::tryFromString(info.baseName());
    FilePath           dst(info.absoluteFilePath());
    FilePath           src =
        uuid ? (mWorkspace.getLibraryDb().*getter)(*uuid) : FilePath();
    if (src.isValid() && !dst.isEmptyDir()) {
      log(QString(tr("Update %1...")).arg(prettyPath(dst)));
      FileUtils::removeDirRecursively(dst);
      FileUtils::copyDirRecursively(src, dst);
    } else {
      log(QString(tr("Skip %1...")).arg(prettyPath(dst)));
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb
