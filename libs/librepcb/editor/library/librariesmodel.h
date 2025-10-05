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

#ifndef LIBREPCB_EDITOR_LIBRARIESMODEL_H
#define LIBREPCB_EDITOR_LIBRARIESMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/network/apiendpoint.h>

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

/*******************************************************************************
 *  Class LibrariesModel
 ******************************************************************************/

/**
 * @brief The LibrariesModel class
 */
class LibrariesModel : public QObject,
                       public slint::Model<ui::LibraryInfoData> {
  Q_OBJECT

public:
  // Types
  enum class Mode { LocalLibs, RemoteLibs };

  // Constructors / Destructor
  LibrariesModel() = delete;
  LibrariesModel(const LibrariesModel& other) = delete;
  explicit LibrariesModel(Workspace& ws, Mode mode,
                          QObject* parent = nullptr) noexcept;
  virtual ~LibrariesModel() noexcept;

  // General Methods
  ui::LibraryListData getUiData() const noexcept;
  void setOnlineVersions(const QHash<Uuid, Version>& versions) noexcept;
  void ensurePopulated(bool withIcons) noexcept;
  void highlightLibraryOnNextRescan(const FilePath& fp) noexcept;
  void checkForUpdates() noexcept;
  void cancelUpdateCheck() noexcept;
  void applyChanges() noexcept;
  void cancel() noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::LibraryInfoData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::LibraryInfoData& data) noexcept override;

  // Operator Overloadings
  LibrariesModel& operator=(const LibrariesModel& rhs) = delete;

signals:
  void uiDataChanged(ui::LibraryListData data);
  void onlineVersionsAvailable(const QHash<Uuid, Version>& versions);
  void aboutToUninstallLibrary(const FilePath& fp);

private:
  void updateLibraries(bool resetHighlight = true) noexcept;
  void requestOnlineLibraries(bool forceNoCache) noexcept;
  void onlineLibraryListReceived(QList<ApiEndpoint::Library> libs) noexcept;
  void requestMissingOnlineIcons() noexcept;
  void onlineIconReceived(const Uuid& uuid, const QByteArray& data) noexcept;
  void errorWhileFetchingLibraryList(QString errorMsg) noexcept;
  void apiEndpointOperationFinished() noexcept;
  void updateMergedLibraries() noexcept;
  void updateCheckStates(bool notify) noexcept;
  void checkMissingDependenciesOfLibs() noexcept;
  void uncheckLibsWithUnmetDependencies() noexcept;
  bool isLibraryChecked(const ui::LibraryInfoData& lib) const noexcept;
  static bool isMarkedForInstall(const ui::LibraryInfoData& lib) noexcept;
  static bool isMarkedForUpdate(const ui::LibraryInfoData& lib) noexcept;
  static bool isMarkedForUninstall(const ui::LibraryInfoData& lib) noexcept;
  std::optional<std::size_t> indexOf(const Uuid& uuid) noexcept;

  Workspace& mWorkspace;
  const Mode mMode;
  bool mInitialized;
  std::vector<ui::LibraryInfoData> mInstalledLibs;  /// Either local or remote
  QStringList mInstalledLibsErrors;
  QHash<Uuid, ApiEndpoint::Library> mOnlineLibs;
  QStringList mOnlineLibsErrors;
  std::vector<ui::LibraryInfoData> mMergedLibs;
  QHash<Uuid, bool> mCheckStates;

  bool mRequestIcons;
  QHash<Uuid, QPixmap> mIcons;
  QHash<slint::SharedString, slint::SharedString> mOnlineVersions;
  std::optional<FilePath> mHighlightedLib;

  QList<std::shared_ptr<ApiEndpoint>> mApiEndpointsInProgress;
  QList<std::shared_ptr<LibraryDownload>> mDownloadsInProgress;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
