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

#include "../apptoolbox.h"
#include "../guiapplication.h"
#include "../workspace/quickaccessmodel.h"
#include "projecteditor.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectloader.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/editor/dialogs/directorylockhandlerdialog.h>
#include <librepcb/editor/dialogs/filedialog.h>
#include <librepcb/editor/workspace/controlpanel/markdownconverter.h>

#include <QtCore>
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

ProjectsModel::ProjectsModel(GuiApplication& app, QObject* parent) noexcept
  : QObject(parent), mApp(app) {
}

ProjectsModel::~ProjectsModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

int ProjectsModel::getIndexOf(std::shared_ptr<ProjectEditor> prj) noexcept {
  return mEditors.indexOf(prj);
}

std::shared_ptr<ProjectEditor> ProjectsModel::getProject(int index) noexcept {
  return mEditors.value(index);
}

std::shared_ptr<ProjectEditor> ProjectsModel::openProject(
    FilePath fp) noexcept {
  if (!fp.isValid()) {
    QSettings cs;  // client settings
    QString lastOpenedFile = cs.value("controlpanel/last_open_project",
                                      mApp.getWorkspace().getPath().toStr())
                                 .toString();

    fp = FilePath(FileDialog::getOpenFileName(
        qApp->activeWindow(), tr("Open Project"), lastOpenedFile,
        tr("LibrePCB project files (%1)").arg("*.lpp *.lppz")));
    if (!fp.isValid()) return nullptr;

    cs.setValue("controlpanel/last_open_project", fp.toNative());
  }

  const QString uniqueFp = fp.toUnique().toStr();
  // if (mEditors.contains(uniqueFp)) return mEditors.value(uniqueFp);

  // QPixmap p =
  // MarkdownConverter::convertMarkdownToPixmap(fp.getParentDir().getPathTo("README.md"),
  // 500); p.save(fp.getParentDir().getPathTo("README.png").toStr());

  // Opening the project can take some time, use wait cursor to provide
  // immediate UI feedback.
  QApplication::setOverrideCursor(Qt::WaitCursor);
  auto cursorScopeGuard =
      scopeGuard([]() { QApplication::restoreOverrideCursor(); });

  try {
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
    std::unique_ptr<Project> project = loader.open(
        std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(fs)),
        projectFileName);  // can throw

    // TODO
    auto schematics =
        std::make_shared<slint::VectorModel<slint::SharedString>>();
    auto boards = std::make_shared<slint::VectorModel<slint::SharedString>>();
    for (auto sch : project->getSchematics()) {
      schematics->push_back(q2s(*sch->getName()));
    }
    for (auto brd : project->getBoards()) {
      boards->push_back(q2s(*brd->getName()));
    }

    // Open editor.
    auto editor = std::make_shared<ProjectEditor>(mApp, std::move(project),
                                                  loader.getUpgradeMessages());

    // Keep handle.
    mEditors.append(editor);
    mItems.push_back(ui::ProjectData{
        true,
        q2s(uniqueFp),
        q2s(*editor->getProject().getName()),
        schematics,
        boards,
    });
    row_added(mItems.size() - 1, 1);

    // Delay updating the last opened project to avoid an issue when
    // double-clicking: https://github.com/LibrePCB/LibrePCB/issues/293
    QTimer::singleShot(500, this, [this, fp]() {
      mApp.getQuickAccess().pushRecentProject(fp);
    });

    return editor;
  } catch (const Exception& e) {
    return nullptr;
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t ProjectsModel::row_count() const {
  return mItems.size();
}

std::optional<ui::ProjectData> ProjectsModel::row_data(std::size_t i) const {
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
