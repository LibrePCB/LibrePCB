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

#ifndef LIBREPCB_LIBRARY_LIBRARIESMODEL_H
#define LIBREPCB_LIBRARY_LIBRARIESMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/network/apiendpoint.h>
#include <librepcb/editor/workspace/librarymanager/librarydownload.h>

#include <QtCore>

#include <vector>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ApiEndpoint;
class Workspace;

namespace editor {

class LibraryDownload;

namespace app {

/*******************************************************************************
 *  Class LibrariesModel
 ******************************************************************************/

/**
 * @brief The LibrariesModel class
 */
class LibrariesModel : public QObject, public slint::Model<ui::Library> {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibrariesModel() = delete;
  LibrariesModel(const LibrariesModel& other) = delete;
  explicit LibrariesModel(Workspace& ws, QObject* parent = nullptr) noexcept;
  virtual ~LibrariesModel() noexcept;

  // General Methods
  void ensurePopulated() noexcept;
  int getOutdatedLibraries() const noexcept;
  int getCheckedLibraries() const noexcept;
  bool isFetchingRemoteLibraries() const noexcept {
    return !mApiEndpointsInProgress.isEmpty();
  }
  void installCheckedLibraries() noexcept;
  void uninstallLibrary(const slint::SharedString& id) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::Library> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i, const ui::Library& obj) noexcept;

  // Operator Overloadings
  LibrariesModel& operator=(const LibrariesModel& rhs) = delete;

signals:
  void outdatedLibrariesChanged(int count);
  void checkedLibrariesChanged(int count);
  void fetchingRemoteLibrariesChanged(bool fetching);

private:
  void refreshLocalLibraries() noexcept;
  void refreshRemoteLibraries() noexcept;
  void onlineLibraryListReceived(QList<ApiEndpoint::Library> libs) noexcept;
  void errorWhileFetchingLibraryList(QString errorMsg) noexcept;
  void refreshMergedLibs() noexcept;
  std::optional<std::size_t> findLib(const QString& id) noexcept;

  Workspace& mWorkspace;
  QList<std::shared_ptr<ApiEndpoint>> mApiEndpointsInProgress;
  QList<std::shared_ptr<LibraryDownload>> mDownloadsInProgress;
  QHash<Uuid, ui::Library> mLocalLibs;
  QHash<Uuid, ApiEndpoint::Library> mRemoteLibs;
  QHash<Uuid, QPixmap> mRemoteIcons;
  std::vector<ui::Library> mMergedLibs;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
