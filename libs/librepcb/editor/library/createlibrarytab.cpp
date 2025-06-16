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
#include "librariesmodel.h"
#include "utils/slinthelpers.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

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
  : WindowTab(app, parent),
    onDerivedUiDataChanged(*this),
    mUiData{
        "My Library",  // Name (not translated by intention)
        slint::SharedString(),  // Name error
        slint::SharedString(),  // Description
        q2s(app.getWorkspace().getSettings().userName.get()),  // Author
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

ui::TabData CreateLibraryTab::getUiData() const noexcept {
  return ui::TabData{
      ui::TabType::CreateLibrary,  // Type
      q2s(tr("New Library")),  // Title
      ui::TabFeatures{},  // Features
      false,  // Read-only
      false,  // Unsaved changes
      slint::SharedString(),  // Undo text
      slint::SharedString(),  // Redo text
      slint::SharedString(),  // Find term
      nullptr,  // Find suggestions
      nullptr,  // Layers
  };
}

void CreateLibraryTab::setDerivedUiData(
    const ui::CreateLibraryTabData& data) noexcept {
  mUiData = data;
  validate();
}

void CreateLibraryTab::trigger(ui::TabAction a) noexcept {
  switch (a) {
    case ui::TabAction::Cancel: {
      emit closeRequested();
      break;
    }

    case ui::TabAction::Accept: {
      try {
        if ((!mName) || (!mVersion) || (!mDirectory.isValid())) {
          throw LogicError(__FILE__, __LINE__);
        }

        // Create transactional file system.
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::openRW(mDirectory);
        TransactionalDirectory dir(fs);

        // Create the new library.
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

        // Copy additional files.
        auto copyFile = [fs](const QString& src, const QString& dst,
                             const QList<std::pair<QByteArray, QByteArray>>&
                                 substitutions = {}) {
          const FilePath srcFp = Application::getResourcesDir().getPathTo(src);
          try {
            QByteArray content = FileUtils::readFile(srcFp);  // can throw
            for (const auto& pair : substitutions) {
              content.replace(pair.first, pair.second);
            }
            fs->write(dst, content);  // can throw
          } catch (const Exception& e) {
            const FilePath dstFp = fs->getAbsPath(dst);
            qCritical().nospace().noquote()
                << "Failed to copy file '" << srcFp.toNative() << "' to '"
                << dstFp.toNative() << "': " << e.getMsg();
          }
        };
        if (mUiData.cc0) {
          copyFile("licenses/cc0-1.0.txt", "LICENSE.txt");
        }
        copyFile(
            "library/readme_template", "README.md",
            {
                {"{LIBRARY_NAME}", (*mName)->toUtf8()},
                {"{LICENSE_TEXT}",
                 mUiData.cc0 ? "Creative Commons (CC0-1.0). For the license "
                               "text, see [LICENSE.txt](LICENSE.txt)."
                             : "No license set."},
            });
        copyFile("library/gitignore_template", ".gitignore");
        copyFile("library/gitattributes_template", ".gitattributes");

        // Save file system.
        fs->save();  // can throw

        // Highlight the new library in the libraries tab.
        emit panelPageRequested(ui::PanelPage::Libraries);
        mApp.getLocalLibraries().highlightLibraryOnNextRescan(mDirectory);

        // Force rescan to index the new library.
        mApp.getWorkspace().getLibraryDb().startLibraryRescan();

        // Close tab as it is no longer required.
        emit closeRequested();
      } catch (const Exception& e) {
        mUiData.creation_error = q2s(e.getMsg());
        onDerivedUiDataChanged.notify();
      }
      break;
    }

    default: {
      WindowTab::trigger(a);
      break;
    }
  }
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
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
