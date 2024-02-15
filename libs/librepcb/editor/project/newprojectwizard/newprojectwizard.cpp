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
#include "newprojectwizard.h"

#include "newprojectwizardpage_eagleimport.h"
#include "newprojectwizardpage_initialization.h"
#include "newprojectwizardpage_metadata.h"
#include "ui_newprojectwizard.h"

#include <librepcb/core/application.h>
#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

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

NewProjectWizard::NewProjectWizard(const Workspace& ws, Mode mode,
                                   QWidget* parent) noexcept
  : QWizard(parent),
    mWorkspace(ws),
    mMode(mode),
    mUi(new Ui::NewProjectWizard),
    mPageEagleImport(nullptr),
    mPageMetadata(nullptr),
    mPageInitialization(nullptr) {
  mUi->setupUi(this);

  if (mMode == Mode::EagleImport) {
    addPage(mPageEagleImport =
                new NewProjectWizardPage_EagleImport(mWorkspace, this));
  }
  addPage(mPageMetadata = new NewProjectWizardPage_Metadata(mWorkspace, this));
  if (mMode == Mode::NewProject) {
    addPage(mPageInitialization =
                new NewProjectWizardPage_Initialization(this));
  }

  if (mPageEagleImport && mPageMetadata) {
    connect(mPageEagleImport,
            &NewProjectWizardPage_EagleImport::projectSelected, mPageMetadata,
            &NewProjectWizardPage_Metadata::setProjectName);
  }

  // Restore client settings.
  QSettings cs;
  const QSize windowSize = cs.value("new_project_wizard/window_size").toSize();
  if (!windowSize.isEmpty()) {
    resize(windowSize);
  }
}

NewProjectWizard::~NewProjectWizard() noexcept {
  // Save client settings.
  QSettings cs;
  cs.setValue("new_project_wizard/window_size", size());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void NewProjectWizard::setLocationOverride(const FilePath& dir) noexcept {
  mPageMetadata->setLocationOverride(dir);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::unique_ptr<Project> NewProjectWizard::createProject() const {
  // Try to remove directory after creation failed and if it doesn't exist now.
  const FilePath projectDir = mPageMetadata->getFullFilePath().getParentDir();
  const bool projectDirExisted = projectDir.isExistingDir();
  auto sg = scopeGuard([projectDir, projectDirExisted]() {
    if (!projectDirExisted) QDir(projectDir.toStr()).removeRecursively();
  });

  // Show wait cursor since this operation can take a while.
  qApp->setOverrideCursor(Qt::WaitCursor);
  auto cursorSg = scopeGuard([]() { qApp->restoreOverrideCursor(); });

  // create file system
  std::shared_ptr<TransactionalFileSystem> fs =
      TransactionalFileSystem::openRW(projectDir);
  TransactionalDirectory dir(fs);

  // create project and set some metadata
  std::unique_ptr<Project> project = Project::create(
      std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(fs)),
      mPageMetadata->getFullFilePath().getFilename());
  project->setName(ElementName(
      cleanElementName(mPageMetadata->getProjectName())));  // can throw
  project->setAuthor(mPageMetadata->getProjectAuthor());

  // set project settings (copy from workspace settings)
  project->setLocaleOrder(mWorkspace.getSettings().libraryLocaleOrder.get());
  project->setNormOrder(mWorkspace.getSettings().libraryNormOrder.get());

  // add schematic
  if (mPageInitialization && mPageInitialization->getCreateSchematic()) {
    Schematic* schematic = new Schematic(
        *project,
        std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory()),
        mPageInitialization->getSchematicDirName(), Uuid::createRandom(),
        ElementName(mPageInitialization->getSchematicName()));  // can throw
    project->addSchematic(*schematic);
  }

  // add board
  if (mPageInitialization && mPageInitialization->getCreateBoard()) {
    Board* board = new Board(
        *project,
        std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory()),
        mPageInitialization->getBoardDirName(), Uuid::createRandom(),
        ElementName(mPageInitialization->getBoardName()));  // can throw
    board->addDefaultContent();
    project->addBoard(*board);
  }

  // copy license file
  if (mPageMetadata->isLicenseSet()) {
    try {
      FilePath source = mPageMetadata->getProjectLicenseFilePath();
      dir.write("LICENSE.txt", FileUtils::readFile(source));  // can throw
    } catch (Exception& e) {
      qCritical() << "Could not copy the license file:" << e.getMsg();
    }
  }

  // copy readme file
  try {
    FilePath source =
        Application::getResourcesDir().getPathTo("project/readme_template");
    QByteArray content = FileUtils::readFile(source);  // can throw
    content.replace("{PROJECT_NAME}", mPageMetadata->getProjectName().toUtf8());
    if (mPageMetadata->isLicenseSet()) {
      content.replace("{LICENSE_TEXT}", "See [LICENSE.txt](LICENSE.txt).");
    } else {
      content.replace("{LICENSE_TEXT}", "No license set.");
    }
    dir.write("README.md", content);  // can throw
  } catch (Exception& e) {
    qCritical() << "Could not copy the readme file:" << e.getMsg();
  }

  // copy .gitignore
  try {
    FilePath source =
        Application::getResourcesDir().getPathTo("project/gitignore_template");
    dir.write(".gitignore", FileUtils::readFile(source));  // can throw
  } catch (Exception& e) {
    qCritical() << "Could not copy the .gitignore file:" << e.getMsg();
  }

  // copy .gitattributes
  try {
    FilePath source = Application::getResourcesDir().getPathTo(
        "project/gitattributes_template");
    dir.write(".gitattributes", FileUtils::readFile(source));  // can throw
  } catch (Exception& e) {
    qCritical() << "Could not copy the .gitattributes file:" << e.getMsg();
  }

  // Import EAGLE project, if needed.
  if (mPageEagleImport) {
    try {
      mPageEagleImport->import(*project);  // can throw
    } catch (const Exception& e) {
      throw RuntimeError(__FILE__, __LINE__,
                         tr("EAGLE import failed:") % "\n\n" % e.getMsg());
    }
  }

  // save project to filesystem
  project->save();  // can throw
  fs->save();  // can throw

  // all done, return the new project
  sg.dismiss();
  return project;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
