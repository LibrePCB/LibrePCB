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
#include "projectlibraryupdater.h"

#include "../controlpanel/controlpanel.h"
#include "ui_projectlibraryupdater.h"

#include <librepcb/common/fileio/transactionalfilesystem.h>
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

using project::Project;
using workspace::WorkspaceLibraryDb;

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
      // open file system
      log(tr("Open project file system..."));
      std::shared_ptr<TransactionalFileSystem> fs =
          TransactionalFileSystem::openRW(
              mProjectFilePath.getParentDir(),
              &TransactionalFileSystem::RestoreMode::abort);

      // update all elements
      updateElements(fs, "cmp", &WorkspaceLibraryDb::getLatestComponent);
      updateElements(fs, "dev", &WorkspaceLibraryDb::getLatestDevice);
      updateElements(fs, "pkg", &WorkspaceLibraryDb::getLatestPackage);
      updateElements(fs, "sym", &WorkspaceLibraryDb::getLatestSymbol);

      // check whether project can still be opened of if we broke something
      try {
        log(tr("Open project %1...").arg(prettyPath(mProjectFilePath)));
        Project project(std::unique_ptr<TransactionalDirectory>(
                            new TransactionalDirectory(fs)),
                        mProjectFilePath.getFilename());
        log(tr("Save project %1...").arg(prettyPath(mProjectFilePath)));
        project.save();  // force updating library elements file format
        fs->save();      // can throw
      } catch (const Exception& e) {
        // something is broken -> discard modifications in file system
        log(tr("[ERROR] %1").arg(e.getMsg()));
        throw RuntimeError(__FILE__, __LINE__,
                           tr("Failed to update library elements! Probably "
                              "there were breaking "
                              "changes in some library elements."));
      }
      log(tr("[SUCCESS] All library elements updated."));
    } catch (const Exception& e) {
      log(tr("[ERROR] %1").arg(e.getMsg()));
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
    std::shared_ptr<TransactionalFileSystem> fs, const QString& type,
    FilePath (workspace::WorkspaceLibraryDb::*getter)(const Uuid&) const) {
  QString dirpath = "library/" % type;
  foreach (const QString& dirname, fs->getDirs(dirpath)) {
    tl::optional<Uuid> uuid = Uuid::tryFromString(dirname);
    FilePath           src =
        uuid ? (mWorkspace.getLibraryDb().*getter)(*uuid) : FilePath();
    QString                dst = dirpath % "/" % dirname;
    TransactionalDirectory dstDir(fs, dst);
    if (src.isValid() && (!dstDir.getFiles().isEmpty())) {
      log(tr("Update %1...").arg(dst));
      std::shared_ptr<TransactionalFileSystem> srcFs =
          TransactionalFileSystem::openRO(src);
      TransactionalDirectory srcDir(srcFs);
      fs->removeDirRecursively(dst);
      srcDir.saveTo(dstDir);
    } else {
      log(tr("Skip %1...").arg(dst));
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb
