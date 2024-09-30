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
#include "librariesmodel.h"

#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/core/network/apiendpoint.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibrariesModel::LibrariesModel(Workspace& ws, QObject* parent) noexcept
  : QObject(parent), mWorkspace(ws) {
  refreshLocalLibraries();
  refreshRemoteLibraries();
}

LibrariesModel::~LibrariesModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::size_t LibrariesModel::row_count() const {
  return mLibs.size();
}

std::optional<ui::Library> LibrariesModel::row_data(std::size_t i) const {
  return (i < mLibs.size()) ? std::optional(mLibs.at(i)) : std::nullopt;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibrariesModel::refreshLocalLibraries() noexcept {
  try {
    QMultiMap<Version, FilePath> libraries =
        mWorkspace.getLibraryDb().getAll<Library>();  // can throw

    foreach (const FilePath& libDir, libraries) {
      Version version = Version::fromString("1");
      mWorkspace.getLibraryDb().getMetadata<Library>(libDir, nullptr,
                                                     &version);  // can throw
      QString name, description, keywords;
      mWorkspace.getLibraryDb().getTranslations<Library>(
          libDir, mWorkspace.getSettings().libraryLocaleOrder.get(), &name,
          &description, &keywords);  // can throw
      QPixmap icon;
      mWorkspace.getLibraryDb().getLibraryMetadata(libDir, &icon);  // can throw

      QImage image = icon.toImage();
      image.convertTo(QImage::Format_RGBA8888);
      slint::SharedPixelBuffer<slint::Rgba8Pixel> img(
          image.width(), image.height(),
          reinterpret_cast<const slint::Rgba8Pixel*>(image.bits()));

      mLibs.push_back(ui::Library{
          libDir.toStr().toUtf8().data(),
          name.toUtf8().data(),
          description.toUtf8().data(),
          img,
          version.toStr().toUtf8().data(),
          true,
          true,
      });
    }

    std::sort(mLibs.begin(), mLibs.end(),
              [](const ui::Library& a, const ui::Library& b) {
                return a.name < b.name;
              });
  } catch (const Exception& e) {
    qCritical() << "Failed to update library list:" << e.getMsg();
  }

  reset();
}

void LibrariesModel::refreshRemoteLibraries() noexcept {
  mApiEndpointsInProgress.clear();  // disconnects all signal/slot connections
  foreach (const QUrl& url, mWorkspace.getSettings().apiEndpoints.get()) {
    std::shared_ptr<ApiEndpoint> repo = std::make_shared<ApiEndpoint>(url);
    //connect(repo.get(), &ApiEndpoint::libraryListReceived, this,
    //        &AddLibraryWidget::onlineLibraryListReceived);
    //connect(repo.get(), &ApiEndpoint::errorWhileFetchingLibraryList, this,
    //        &AddLibraryWidget::errorWhileFetchingLibraryList);
    repo->requestLibraryList();
    mApiEndpointsInProgress.append(repo);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
