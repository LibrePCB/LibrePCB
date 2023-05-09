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

#include "../../project/projecteditor.h"
#include "../controlpanel/controlpanel.h"
#include "ui_projectlibraryupdater.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/fileio/versionfile.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectloader.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectLibraryUpdater::ProjectLibraryUpdater(Workspace& ws,
                                             const FilePath& project,
                                             ControlPanel& cp) noexcept
  : QDialog(nullptr),
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
  bool abort = false;
  ProjectEditor* editor = mControlPanel.getOpenProject(mProjectFilePath);
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

      // Abort if the file format is outdated because it would lead to errors
      // when library elements with a higher file format version get copied
      // into the project. The user shall first perform a file format upgrade
      // and review the changes before upgrading the project library.
      const Version fileFormat =
          VersionFile::fromByteArray(fs->read(".librepcb-project"))
              .getVersion();
      if (fileFormat < Application::getFileFormatVersion()) {
        throw RuntimeError(
            __FILE__, __LINE__,
            tr("The project uses an outdated file format.\nPlease upgrade it "
               "to the latest file format first, review the upgrade messages "
               "and then save the project.\nAfterwards the project library can "
               "be updated."));
      }

      // update all elements
      updateElements<Component>(fs, "cmp");
      updateElements<Device>(fs, "dev");
      updateElements<Package>(fs, "pkg");
      updateElements<Symbol>(fs, "sym");

      // check whether project can still be opened of if we broke something
      try {
        log(tr("Open project %1...").arg(prettyPath(mProjectFilePath)));
        ProjectLoader loader;
        loader.setAutoAssignDeviceModels(true);  // Make use of new 3D models.
        std::unique_ptr<Project> project =
            loader.open(std::unique_ptr<TransactionalDirectory>(
                            new TransactionalDirectory(fs)),
                        mProjectFilePath.getFilename());  // can throw
        log(tr("Save project %1...").arg(prettyPath(mProjectFilePath)));
        project->save();  // force upgrading file format
        fs->save();  // can throw
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
      QTimer::singleShot(500, this, &QDialog::raise);
      QTimer::singleShot(500, this, &QDialog::activateWindow);
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

template <typename T>
void ProjectLibraryUpdater::updateElements(
    std::shared_ptr<TransactionalFileSystem> fs, const QString& type) {
  QString dirpath = "library/" % type;
  foreach (const QString& dirname, fs->getDirs(dirpath)) {
    tl::optional<Uuid> uuid = Uuid::tryFromString(dirname);
    FilePath src =
        uuid ? mWorkspace.getLibraryDb().getLatest<T>(*uuid) : FilePath();
    QString dst = dirpath % "/" % dirname;
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

}  // namespace editor
}  // namespace librepcb
