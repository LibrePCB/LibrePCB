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

#include "newprojectwizardpage_initialization.h"
#include "newprojectwizardpage_metadata.h"
#include "ui_newprojectwizard.h"

#include <librepcb/common/application.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/fileio/transactionalfilesystem.h>
#include <librepcb/project/metadata/projectmetadata.h>
#include <librepcb/project/project.h>
#include <librepcb/project/settings/projectsettings.h>
#include <librepcb/projecteditor/licenses/license_base.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

//#include "newprojectwizardpage_versioncontrol.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NewProjectWizard::NewProjectWizard(const workspace::Workspace& ws,
                                   QWidget*                    parent) noexcept
  : QWizard(parent), mWorkspace(ws), mUi(new Ui::NewProjectWizard) {
  mUi->setupUi(this);

  addPage(mPageMetadata = new NewProjectWizardPage_Metadata(mWorkspace, this));
  addPage(mPageInitialization = new NewProjectWizardPage_Initialization(this));
  // addPage(mPageVersionControl = new
  // NewProjectWizardPage_VersionControl(this));
}

NewProjectWizard::~NewProjectWizard() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void NewProjectWizard::setLocation(const FilePath& dir) noexcept {
  mPageMetadata->setDefaultLocation(dir);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

Project* NewProjectWizard::createProject() const {
  // create file system
  std::shared_ptr<TransactionalFileSystem> fs = TransactionalFileSystem::openRW(
      mPageMetadata->getFullFilePath().getParentDir());
  TransactionalDirectory dir(fs);

  // create project and set some metadata
  QScopedPointer<Project> project(Project::create(
      std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(fs)),
      mPageMetadata->getFullFilePath().getFilename()));
  project->getMetadata().setName(
      ElementName(mPageMetadata->getProjectName().trimmed()));  // can throw
  project->getMetadata().setAuthor(mPageMetadata->getProjectAuthor());

  // set project settings (copy from workspace settings)
  ProjectSettings& settings = project->getSettings();
  settings.setLocaleOrder(mWorkspace.getSettings().libraryLocaleOrder.get());
  settings.setNormOrder(mWorkspace.getSettings().libraryNormOrder.get());

  // add schematic
  if (mPageInitialization->getCreateSchematic()) {
    Schematic* schematic = project->createSchematic(
        ElementName(mPageInitialization->getSchematicName()));  // can throw
    project->addSchematic(*schematic);
  }

  // add board
  if (mPageInitialization->getCreateBoard()) {
    Board* board = project->createBoard(
        ElementName(mPageInitialization->getBoardName()));  // can throw
    project->addBoard(*board);
  }

  // apply license
  if (mPageMetadata->isLicenseSet()) {
    std::unique_ptr<LicenseBase> license = mPageMetadata->getProjectLicense();
    if (license != nullptr) {
      // copy all license files
      const QList<std::pair<QString, QString>>& files = license->getFiles();
      for (int i = 0; i < files.length(); i++) {
        FilePath sourcePath =
            qApp->getResourcesDir().getPathTo("licenses/").getPathTo(files.at(i).first);
        const QString& destFilename = files.at(i).second;
        try {
          dir.write(destFilename,
                    FileUtils::readFile(sourcePath));  // can throw
        } catch (Exception& e) {
          qCritical() << "Could not copy the license file:" << e.getMsg();
        }
      }
    }
  }

  // copy readme file
  try {
    FilePath source =
        qApp->getResourcesDir().getPathTo("project/readme_template");
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
        qApp->getResourcesDir().getPathTo("project/gitignore_template");
    dir.write(".gitignore", FileUtils::readFile(source));  // can throw
  } catch (Exception& e) {
    qCritical() << "Could not copy the .gitignore file:" << e.getMsg();
  }

  // copy .gitattributes
  try {
    FilePath source =
        qApp->getResourcesDir().getPathTo("project/gitattributes_template");
    dir.write(".gitattributes", FileUtils::readFile(source));  // can throw
  } catch (Exception& e) {
    qCritical() << "Could not copy the .gitattributes file:" << e.getMsg();
  }

  // save project to filesystem
  project->save();  // can throw
  fs->save();       // can throw

  // all done, return the new project
  return project.take();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
