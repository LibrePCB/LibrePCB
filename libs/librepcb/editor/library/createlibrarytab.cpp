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
#include "createlibrarytab.h"

#include "guiapplication.h"
#include "utils/editortoolbox.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
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

CreateLibraryTab::CreateLibraryTab(GuiApplication& app,
                                   QObject* parent) noexcept
  : WindowTab(app, nullptr, -1, parent),
    mUiData{
        q2s(tr("My Library")),  // Name
        slint::SharedString(),  // Name error
        slint::SharedString(),  // Description
        slint::SharedString(),  // Author
        q2s(app.getWorkspace().getSettings().userName.get()),  // Author default
        slint::SharedString(),  // Version
        "0.1",  // Version default
        slint::SharedString(),  // Version error
        slint::SharedString(),  // URL
        slint::SharedString(),  // URL error
        false,  // CC0
        slint::SharedString(),  // Directory
        slint::SharedString(),  // Directory default
        slint::SharedString(),  // Directory error
        false,  // Valid
        slint::SharedString(),  // Creation error
    } {
  validate();
}

CreateLibraryTab::~CreateLibraryTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::TabData CreateLibraryTab::getBaseUiData() const noexcept {
  return ui::TabData{
      ui::TabType::CreateLibrary,  // Type
      q2s(tr("New Library")),  // Title
      -1,  // Project index
      ui::RuleCheckState::NotAvailable,  // Rule check state
      nullptr,  // Rule check messages
      0,  // Rule check unapproved messages
      slint::SharedString(),  // Rule check execution error
      false,  // Can save
      false,  // Can export graphics
      false,  // Can undo
      slint::SharedString(),  // Undo text
      false,  // Can redo
      slint::SharedString(),  // Redo text
      false,  // Can cut
      false,  // Can copy
      false,  // Can paste
      false,  // Can remove
      false,  // Can rotate
      false,  // Can mirror
      false,  // Can move/align
      false,  // Can snap to grid
      false,  // Can reset texts
      false,  // Can lock placement
      false,  // Can edit properties
  };
}

void CreateLibraryTab::setUiData(
    const ui::CreateLibraryTabData& data) noexcept {
  mUiData = data;
  validate();
}

void CreateLibraryTab::activate() noexcept {
}

void CreateLibraryTab::deactivate() noexcept {
}

bool CreateLibraryTab::trigger(ui::Action a) noexcept {
  if (a == ui::Action::SectionOk) {
    try {
      if ((!mName) || (!mVersion) || (!mDirectory.isValid())) {
        throw LogicError(__FILE__, __LINE__);
      }

      // create transactional file system
      std::shared_ptr<TransactionalFileSystem> fs =
          TransactionalFileSystem::openRW(mDirectory);
      TransactionalDirectory dir(fs);

      // create the new library
      QScopedPointer<Library> lib(new Library(
          Uuid::createRandom(), *mVersion, s2q(mUiData.author).trimmed(),
          *mName, s2q(mUiData.description).trimmed(),
          QString("")));  // can throw
      if (mUrl) {
        lib->setUrl(*mUrl);
      }
      try {
        lib->setIcon(
            FileUtils::readFile(Application::getResourcesDir().getPathTo(
                "library/default_image.png")));
      } catch (const Exception& e) {
        qCritical() << "Could not open the library image:" << e.getMsg();
      }
      lib->moveTo(dir);  // can throw

      // copy license file
      if (mUiData.cc0) {
        try {
          FilePath source =
              Application::getResourcesDir().getPathTo("licenses/cc0-1.0.txt");
          fs->write("LICENSE.txt", FileUtils::readFile(source));  // can throw
        } catch (Exception& e) {
          qCritical() << "Could not copy the license file:" << e.getMsg();
        }
      }

      // copy readme file
      try {
        FilePath source =
            Application::getResourcesDir().getPathTo("library/readme_template");
        QByteArray content = FileUtils::readFile(source);  // can throw
        content.replace("{LIBRARY_NAME}", (*mName)->toUtf8());
        if (mUiData.cc0) {
          content.replace("{LICENSE_TEXT}",
                          "Creative Commons (CC0-1.0). For the "
                          "license text, see [LICENSE.txt](LICENSE.txt).");
        } else {
          content.replace("{LICENSE_TEXT}", "No license set.");
        }
        fs->write("README.md", content);  // can throw
      } catch (Exception& e) {
        qCritical() << "Could not copy the readme file:" << e.getMsg();
      }

      // copy .gitignore
      try {
        FilePath source = Application::getResourcesDir().getPathTo(
            "library/gitignore_template");
        fs->write(".gitignore", FileUtils::readFile(source));  // can throw
      } catch (Exception& e) {
        qCritical() << "Could not copy the .gitignore file:" << e.getMsg();
      }

      // copy .gitattributes
      try {
        FilePath source = Application::getResourcesDir().getPathTo(
            "library/gitattributes_template");
        fs->write(".gitattributes", FileUtils::readFile(source));  // can throw
      } catch (Exception& e) {
        qCritical() << "Could not copy the .gitattributes file:" << e.getMsg();
      }

      // save file system
      fs->save();  // can throw

      // Force rescan to index the new library.
      mApp.getWorkspace().getLibraryDb().startLibraryRescan();

      // Close tab as it is no longer required.
      emit requestClose();
    } catch (const Exception& e) {
      mUiData.creation_error = q2s(e.getMsg());
      emit tabUiDataChanged();
    }
  }

  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CreateLibraryTab::validate() noexcept {
  const QString nameStr = s2q(mUiData.name).remove(".lplib");
  mName = validateElementName(nameStr, mUiData.name_error);

  QString versionStr = s2q(mUiData.version).trimmed();
  if (versionStr.isEmpty()) {
    versionStr = s2q(mUiData.version_default);
  }
  mVersion = validateVersion(versionStr, mUiData.version_error);

  mUrl = validateUrl(s2q(mUiData.url), mUiData.url_error, true);

  QString dirDefault = FilePath::cleanFileName(
      nameStr, FilePath::ReplaceSpaces | FilePath::KeepCase);
  if (!dirDefault.isEmpty()) {
    dirDefault.append(".lplib");
  }
  mUiData.directory_default = q2s(dirDefault);

  QString dirStr = s2q(mUiData.directory).trimmed();
  if (dirStr.isEmpty()) {
    dirStr = s2q(mUiData.directory_default);
  }
  const std::optional<FileProofName> dirName =
      validateFileProofName(dirStr, mUiData.directory_error, ".lplib");
  mDirectory = dirName
      ? mApp.getWorkspace().getLibrariesPath().getPathTo("local/" % *dirName)
      : FilePath();
  if (mDirectory.isValid() &&
      (mDirectory.isExistingFile() || mDirectory.isExistingDir())) {
    mDirectory = FilePath();
    mUiData.directory_error = q2s(tr("Exists already"));
  }

  mUiData.valid =
      mName && mVersion && mUiData.url_error.empty() && mDirectory.isValid();
  emit tabUiDataChanged();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
