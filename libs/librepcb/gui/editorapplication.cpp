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
#include "editorapplication.h"

#include "editorwindow.h"
#include "objectlistmodel.h"
#include "openedproject.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectloader.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/editor/dialogs/directorylockhandlerdialog.h>
#include <librepcb/editor/dialogs/filedialog.h>
#include <librepcb/editor/project/newprojectwizard/newprojectwizard.h>
#include <librepcb/editor/workspace/workspacesettingsdialog.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace gui {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

EditorApplication::EditorApplication(Workspace& ws, QObject* parent)
  : QObject(parent),
    mWorkspace(ws),
    mWindows(),
    mOpenedProjects(new ObjectListModel(this)) {
  mWindows.append(std::make_shared<EditorWindow>(*this));

  // Slightly delay opening projects just to ensure the GUI is fully loaded.
  QTimer::singleShot(20, this,
                     &EditorApplication::openProjectsPassedByCommandLine);
}

EditorApplication::~EditorApplication() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString EditorApplication::getWorkspacePath() const noexcept {
  return mWorkspace.getPath().toNative();
}

QAbstractItemModel* EditorApplication::getOpenedProjects() noexcept {
  return mOpenedProjects.data();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<OpenedProject> EditorApplication::createProject() noexcept {
  editor::NewProjectWizard wizard(mWorkspace,
                                  editor::NewProjectWizard::Mode::NewProject);
  wizard.setLocationOverride(mWorkspace.getProjectsPath());
  if (wizard.exec() == QWizard::Accepted) {
    try {
      std::unique_ptr<Project> project = wizard.createProject();  // can throw
      const FilePath fp = project->getFilepath();
      project.reset();  // Release lock.
      return openProject(fp);
    } catch (const Exception& e) {
      QMessageBox::critical(nullptr, tr("Could not create project"),
                            e.getMsg());
    }
  }
  return nullptr;
}

std::shared_ptr<OpenedProject> EditorApplication::openProject() noexcept {
  QSettings s;
  const QString lastOpenedFile =
      s.value("app/last_open_project", mWorkspace.getPath().toStr()).toString();
  const FilePath fp = FilePath(editor::FileDialog::getOpenFileName(
      nullptr, tr("Open Project"), lastOpenedFile,
      tr("LibrePCB project files (%1)").arg("*.lpp *.lppz")));
  if (!fp.isValid()) return nullptr;
  s.setValue("app/last_open_project", fp.toNative());
  return openProject(fp);
}

/*******************************************************************************
 *  GUI Handlers
 ******************************************************************************/

void EditorApplication::openWorkspaceSettings() noexcept {
  editor::WorkspaceSettingsDialog dialog(mWorkspace);
  // connect(&dialog,
  //        &editor::WorkspaceSettingsDialog::desktopIntegrationStatusChanged,
  //        this, &ControlPanel::updateDesktopIntegrationMessage);
  dialog.exec();
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

bool EditorApplication::eventFilter(QObject* watched, QEvent* event) noexcept {
  if (event->type() == QEvent::FileOpen) {
    QFileOpenEvent* openEvent = static_cast<QFileOpenEvent*>(event);
    qDebug() << "Received request to open file:" << openEvent->file();
    openProjectPassedByOs(openEvent->file());
    return true;
  }
  return QObject::eventFilter(watched, event);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void EditorApplication::openProjectsPassedByCommandLine() noexcept {
  // Parse command line arguments and open all project files.
  // Note: Do not print a warning if the first argument is not a valid project,
  // since it might or might not be the application file path.
  const QStringList args = qApp->arguments();
  for (int i = 0; i < args.count(); ++i) {
    openProjectPassedByOs(args.at(i), i == 0);  // Silent on first item.
  }
}

void EditorApplication::openProjectPassedByOs(const QString& file,
                                              bool silent) noexcept {
  FilePath fp(file);
  if ((fp.isExistingFile()) &&
      ((fp.getSuffix() == "lpp") || (fp.getSuffix() == "lppz"))) {
    openProject(fp);
  } else if (!silent) {
    qWarning() << "Ignore invalid request to open project:" << file;
  }
}

std::shared_ptr<OpenedProject> EditorApplication::openProject(
    const FilePath& fp) noexcept {
  try {
    // Opening the project can take some time, use wait cursor to provide
    // immediate UI feedback.
    qApp->setOverrideCursor(Qt::WaitCursor);
    auto cursorScopeGuard =
        scopeGuard([this]() { qApp->restoreOverrideCursor(); });

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
      auto askForRestoringBackup = [](const FilePath& dir) {
        Q_UNUSED(dir);
        QMessageBox::StandardButton btn = QMessageBox::question(
            nullptr, tr("Restore autosave backup?"),
            tr("It seems that the application crashed the last time you opened "
               "this "
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
      };
      fs = TransactionalFileSystem::openRW(
          fp.getParentDir(), askForRestoringBackup,
          editor::DirectoryLockHandlerDialog::createDirectoryLockCallback());
    }
    ProjectLoader loader;
    std::unique_ptr<Project> project = loader.open(
        std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(fs)),
        projectFileName);  // can throw
    auto openedProject =
        std::make_shared<OpenedProject>(*this, std::move(project));
    mOpenedProjects->insert(-1, openedProject);
    return openedProject;
  } catch (const UserCanceled&) {
  } catch (const Exception& e) {
    QMessageBox::critical(nullptr, tr("Could not open project"), e.getMsg());
  }
  return nullptr;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace gui
}  // namespace librepcb
