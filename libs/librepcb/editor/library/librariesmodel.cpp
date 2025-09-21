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

#include "../utils/slinthelpers.h"
#include "librarydownload.h"

#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/network/apiendpoint.h>
#include <librepcb/core/network/networkrequest.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>
#include <QtWidgets>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibrariesModel::LibrariesModel(Workspace& ws, Mode mode,
                               QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mMode(mode),
    mInitialized(false),
    mRequestIcons(false) {
  connect(&mWorkspace.getLibraryDb(),
          &WorkspaceLibraryDb::scanLibraryListUpdated, this,
          &LibrariesModel::updateLibraries, Qt::QueuedConnection);

  QTimer::singleShot(1000, this, [this]() { ensurePopulated(false); });

  // When the list of API endpoints is modified, re-fetch all remote libraries.
  if (mMode == Mode::RemoteLibs) {
    connect(&mWorkspace.getSettings().apiEndpoints,
            &WorkspaceSettingsItem::edited, this, [this]() {
              mApiEndpointsInProgress.clear();
              mOnlineLibsErrors.clear();
              mOnlineLibs.clear();
              updateMergedLibraries();
              ensurePopulated(false);
            });
  }
}

LibrariesModel::~LibrariesModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::LibraryListData LibrariesModel::getUiData() const noexcept {
  ui::LibraryListData data = {};
  data.refreshing = (!mInitialized) || (!mApiEndpointsInProgress.isEmpty());
  data.refreshing_error =
      q2s((mInstalledLibsErrors + mOnlineLibsErrors).join("\n\n"));
  data.count = mMergedLibs.size();
  data.installed = mInstalledLibs.size();
  for (auto& lib : mMergedLibs) {
    if (lib.outdated) {
      ++data.outdated;
    }
    if (isMarkedForInstall(lib)) {
      ++data.pending_installs;
    } else if (isMarkedForUpdate(lib)) {
      ++data.pending_updates;
    } else if (isMarkedForUninstall(lib)) {
      ++data.pending_uninstalls;
      if (lib.online_version.empty()) {
        ++data.pending_oneway_uninstalls;
      }
    }
  }
  data.all_up_to_date =
      (data.installed > 0) && (data.outdated == 0) && (!mOnlineLibs.isEmpty());
  data.operation_in_progress = !mDownloadsInProgress.isEmpty();
  data.operation_error = slint::SharedString();  // TODO
  return data;
}

void LibrariesModel::setOnlineVersions(
    const QHash<Uuid, Version>& versions) noexcept {
  bool modified = false;
  for (auto it = versions.begin(); it != versions.end(); ++it) {
    const auto uuidStr = q2s(it.key().toStr());
    const auto versionStr = q2s(it.value().toStr());
    if (mOnlineVersions.value(uuidStr) != versionStr) {
      mOnlineVersions[uuidStr] = versionStr;
      modified = true;
    }
  }

  if (modified) {
    for (std::size_t i = 0; i < mMergedLibs.size(); ++i) {
      const auto versionStr = mOnlineVersions.value(mMergedLibs.at(i).uuid);
      if ((!versionStr.empty()) &&
          (mMergedLibs.at(i).online_version != versionStr)) {
        mMergedLibs.at(i).online_version = versionStr;
        mMergedLibs.at(i).outdated = Version::fromString(s2q(versionStr)) >
            Version::fromString(s2q(mMergedLibs.at(i).installed_version));
        notify_row_changed(i);
      }
    }
  }
}

void LibrariesModel::ensurePopulated(bool withIcons) noexcept {
  if (withIcons) {
    mRequestIcons = true;
  }
  if (mInstalledLibs.empty() || (!mInstalledLibsErrors.isEmpty())) {
    updateLibraries(false);
  }
  if ((mMode == Mode::RemoteLibs) &&
      (mApiEndpointsInProgress.isEmpty() &&
       (mOnlineLibs.isEmpty() || (!mOnlineLibsErrors.isEmpty())))) {
    requestOnlineLibraries();
  }
  if (mRequestIcons) {
    requestMissingOnlineIcons();
  }
}

void LibrariesModel::highlightLibraryOnNextRescan(const FilePath& fp) noexcept {
  mHighlightedLib = fp;
}

void LibrariesModel::applyChanges() noexcept {
  if (mMode != Mode::RemoteLibs) return;

  // Show wait cursor since some operations can take a while.
  qApp->setOverrideCursor(Qt::WaitCursor);
  auto cursorSg = scopeGuard([]() { qApp->restoreOverrideCursor(); });

  int installed = 0;
  int uninstalled = 0;
  for (auto& lib : mMergedLibs) {
    const auto uuid = Uuid::tryFromString(s2q(lib.uuid));
    if (!uuid) continue;

    if (isMarkedForInstall(lib) || isMarkedForUpdate(lib)) {
      // Install/update library.
      const auto onlineLib = mOnlineLibs.find(*uuid);
      if (onlineLib == mOnlineLibs.end()) continue;

      // Determine destination directory.
      const FilePath destDir = mWorkspace.getLibrariesPath().getPathTo(
          "remote/" % onlineLib->uuid.toStr() % ".lplib");

      // Start download.
      auto dl =
          std::make_shared<LibraryDownload>(onlineLib->downloadUrl, destDir);
      if (onlineLib->downloadSize > 0) {
        dl->setExpectedZipFileSize(onlineLib->downloadSize);
      }
      if (!onlineLib->downloadSha256.isEmpty()) {
        dl->setExpectedChecksum(QCryptographicHash::Sha256,
                                QByteArray::fromHex(onlineLib->downloadSha256));
      }
      connect(
          dl.get(), &LibraryDownload::progressPercent, this,
          [this, uuid](int percent) {
            if (auto i = indexOf(*uuid)) {
              mMergedLibs.at(*i).progress = percent;
              notify_row_changed(*i);
            }
          },
          Qt::QueuedConnection);
      connect(
          dl.get(), &LibraryDownload::finished, this,
          [this, uuid, dl](bool success, const QString& errMsg) {
            Q_UNUSED(success);
            Q_UNUSED(errMsg);
            mDownloadsInProgress.removeOne(dl);
            if (mDownloadsInProgress.isEmpty()) {
              mWorkspace.getLibraryDb().startLibraryRescan();
            }
          },
          Qt::QueuedConnection);
      mDownloadsInProgress.append(dl);
      dl->start();
      ++installed;
    } else if (isMarkedForUninstall(lib)) {
      // Uninstall library.
      try {
        const FilePath fp(s2q(lib.path));
        emit aboutToUninstallLibrary(fp);  // Let the app close it if needed
        FileUtils::removeDirRecursively(fp);  // can throw
      } catch (const Exception& e) {
        // TODO: This should be implemented without message box some day...
        QMessageBox::critical(nullptr, tr("Error"), e.getMsg());
      }
      ++uninstalled;
    }
  }

  if ((installed == 0) && (uninstalled > 0)) {
    mWorkspace.getLibraryDb().startLibraryRescan();
  }

  emit uiDataChanged(getUiData());
}

void LibrariesModel::cancel() noexcept {
  if (!mDownloadsInProgress.isEmpty()) {
    mDownloadsInProgress.clear();
    emit uiDataChanged(getUiData());
  } else if (!mApiEndpointsInProgress.isEmpty()) {
    mApiEndpointsInProgress.clear();
    emit uiDataChanged(getUiData());
  } else {
    mCheckStates.clear();
    updateMergedLibraries();
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t LibrariesModel::row_count() const {
  return mMergedLibs.size();
}

std::optional<ui::LibraryInfoData> LibrariesModel::row_data(
    std::size_t i) const {
  return (i < mMergedLibs.size()) ? std::optional(mMergedLibs.at(i))
                                  : std::nullopt;
}

void LibrariesModel::set_row_data(std::size_t i,
                                  const ui::LibraryInfoData& data) noexcept {
  if (i >= mMergedLibs.size()) return;

  // Show wait cursor since some operations can take a while.
  qApp->setOverrideCursor(Qt::WaitCursor);
  auto cursorSg = scopeGuard([]() { qApp->restoreOverrideCursor(); });

  const auto uuid = Uuid::tryFromString(s2q(data.uuid));
  if (uuid && (data.checked != mMergedLibs.at(i).checked)) {
    mCheckStates[*uuid] = data.checked;
    if (data.checked) {
      checkMissingDependenciesOfLibs();
    } else {
      uncheckLibsWithUnmetDependencies();
    }
    updateCheckStates(true);
    emit uiDataChanged(getUiData());
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibrariesModel::updateLibraries(bool resetHighlight) noexcept {
  mInstalledLibs.clear();
  mInstalledLibsErrors.clear();

  try {
    QMultiMap<Version, FilePath> libraries =
        mWorkspace.getLibraryDb().getAll<Library>();  // can throw

    QSet<Uuid> uuids;
    foreach (const FilePath& libDir, libraries) {
      Uuid uuid = Uuid::createRandom();
      Version version = Version::fromString("1");
      mWorkspace.getLibraryDb().getMetadata<Library>(libDir, &uuid,
                                                     &version);  // can throw

      QPixmap icon;
      mWorkspace.getLibraryDb().getLibraryMetadata(libDir, &icon);  // can throw
      if (!icon.isNull()) {
        mIcons[uuid] = icon;  // Collect local icons to avoid requiring download
      }

      const bool isRemoteLib =
          libDir.isLocatedInDir(mWorkspace.getRemoteLibrariesPath());
      if (isRemoteLib != (mMode == Mode::RemoteLibs)) {
        continue;
      }

      QString name, description, keywords;
      mWorkspace.getLibraryDb().getTranslations<Library>(
          libDir, mWorkspace.getSettings().libraryLocaleOrder.get(), &name,
          &description, &keywords);  // can throw

      const auto sName = q2s(name);
      mInstalledLibs.push_back(ui::LibraryInfoData{
          uuids.contains(uuid)
              ? slint::SharedString()
              : q2s(uuid.toStr()),  // UUID (empty for duplicates)
          q2s(libDir.toNative()),  // Path
          q2s(icon),  // Icon
          sName,  // Name
          q2s(description),  // Description
          q2s(version.toStr()),  // Installed version
          slint::SharedString(),  // Online version
          false,  // Outdated
          false,  // Recommended
          0,  // Progress [%]
          true,  // Checked
          mHighlightedLib == libDir,  // Highlight
      });
      uuids.insert(uuid);
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to update library list:" << e.getMsg();
    mInstalledLibsErrors.append(e.getMsg());
  }

  std::sort(mInstalledLibs.begin(), mInstalledLibs.end(),
            [](const ui::LibraryInfoData& a, const ui::LibraryInfoData& b) {
              return a.name < b.name;
            });

  mInitialized = true;
  updateMergedLibraries();

  if (resetHighlight) {
    mHighlightedLib = std::nullopt;
  }
}

void LibrariesModel::requestOnlineLibraries() noexcept {
  mApiEndpointsInProgress.clear();  // Disconnects all signal/slot connections.
  mOnlineLibs.clear();
  mOnlineLibsErrors.clear();
  for (const WorkspaceSettings::ApiEndpoint& ep :
       mWorkspace.getSettings().apiEndpoints.get()) {
    if (ep.url.isValid() && ep.useForLibraries) {
      std::shared_ptr<ApiEndpoint> repo = std::make_shared<ApiEndpoint>(ep.url);
      connect(repo.get(), &ApiEndpoint::libraryListReceived, this,
              &LibrariesModel::onlineLibraryListReceived);
      connect(repo.get(), &ApiEndpoint::errorWhileFetchingLibraryList, this,
              &LibrariesModel::errorWhileFetchingLibraryList);
      mApiEndpointsInProgress.append(repo);
      repo->requestLibraryList();
    }
  }
  if (!mApiEndpointsInProgress.isEmpty()) {
    emit uiDataChanged(getUiData());
  }
}

void LibrariesModel::onlineLibraryListReceived(
    QList<ApiEndpoint::Library> libs) noexcept {
  QHash<Uuid, Version> versions;
  for (const auto& lib : libs) {
    const Uuid uuid = lib.uuid;
    mOnlineLibs.insert(uuid, lib);
    versions.insert(uuid, lib.version);
  }
  apiEndpointOperationFinished();
  updateMergedLibraries();
  emit onlineVersionsAvailable(versions);

  if (mRequestIcons) {
    requestMissingOnlineIcons();
  }
}

void LibrariesModel::requestMissingOnlineIcons() noexcept {
  for (const ApiEndpoint::Library& lib : mOnlineLibs) {
    if (!mIcons.contains(lib.uuid)) {
      const Uuid uuid = lib.uuid;
      mIcons[uuid] = QPixmap();  // Mark as "requested".
      NetworkRequest* request = new NetworkRequest(lib.iconUrl);
      request->setMinimumCacheTime(24 * 3600);  // 1 day
      connect(request, &NetworkRequest::dataReceived, this,
              [this, uuid](const QByteArray& data) {
                onlineIconReceived(uuid, data);
              });
      request->start();
    }
  }
}

void LibrariesModel::onlineIconReceived(const Uuid& uuid,
                                        const QByteArray& data) noexcept {
  QPixmap pixmap;
  pixmap.loadFromData(data);
  if (pixmap.isNull()) return;
  mIcons[uuid] = pixmap;
  for (std::size_t i = 0; i < mMergedLibs.size(); ++i) {
    if (mMergedLibs.at(i).uuid == uuid.toStr()) {
      mMergedLibs.at(i).icon = q2s(pixmap);
      notify_row_changed(i);
    }
  }
}

void LibrariesModel::errorWhileFetchingLibraryList(QString errorMsg) noexcept {
  const ApiEndpoint* endpoint = qobject_cast<ApiEndpoint*>(sender());
  mOnlineLibsErrors.append(
      tr("Failed to fetch libraries from '%1': %2")
          .arg(endpoint ? endpoint->getUrl().toString() : QString())
          .arg(errorMsg));
  apiEndpointOperationFinished();
  emit uiDataChanged(getUiData());
}

void LibrariesModel::apiEndpointOperationFinished() noexcept {
  const ApiEndpoint* endpoint = qobject_cast<ApiEndpoint*>(sender());
  mApiEndpointsInProgress.removeIf([endpoint](std::shared_ptr<ApiEndpoint> ep) {
    return ep.get() == endpoint;
  });
  endpoint = nullptr;  // Not valid anymore!
  if (mApiEndpointsInProgress.isEmpty()) {
    emit uiDataChanged(getUiData());
  }
}

void LibrariesModel::updateMergedLibraries() noexcept {
  mMergedLibs = mInstalledLibs;
  for (auto& lib : mMergedLibs) {
    lib.online_version = mOnlineVersions.value(lib.uuid);
  }

  for (const auto& lib : mOnlineLibs) {
    bool isInstalled = false;
    for (std::size_t i = 0; i < mInstalledLibs.size(); ++i) {
      auto& installedLib = mMergedLibs.at(i);
      if (installedLib.uuid == lib.uuid.toStr()) {
        installedLib.online_version = q2s(lib.version.toStr());
        installedLib.outdated =
            (lib.version >
             Version::fromString(s2q(installedLib.installed_version)));
        installedLib.recommended = lib.recommended;
        isInstalled = true;
      }
    }
    if (!isInstalled) {
      auto name = q2s(lib.name);
      mMergedLibs.push_back(ui::LibraryInfoData{
          q2s(lib.uuid.toStr()),  // UUID
          slint::SharedString(),  // Path
          q2s(mIcons.value(lib.uuid)),  // Icon
          name,  // Name
          q2s(lib.description),  // Description
          slint::SharedString(),  // Installed version
          q2s(lib.version.toStr()),  // Online version
          false,  // Outdated
          lib.recommended,  // Recommended
          0,  // Progress
          false,  // Checked
          false,  // Highlight
      });
    }
  }

  checkMissingDependenciesOfLibs();
  updateCheckStates(false);

  notify_reset();

  emit uiDataChanged(getUiData());
}

void LibrariesModel::updateCheckStates(bool notify) noexcept {
  for (std::size_t i = 0; i < mMergedLibs.size(); ++i) {
    auto& lib = mMergedLibs.at(i);
    const bool checked = isLibraryChecked(lib);
    if (lib.checked != checked) {
      lib.checked = checked;
      if (notify) {
        notify_row_changed(i);
      }
    }
  }
}

void LibrariesModel::checkMissingDependenciesOfLibs() noexcept {
  QSet<Uuid> toBeInstalled;
  for (const auto& lib : mMergedLibs) {
    const auto uuid = Uuid::tryFromString(s2q(lib.uuid));
    if (uuid && isLibraryChecked(lib)) {
      toBeInstalled.insert(*uuid);
    }
  }

  while (true) {
    const int oldCount = toBeInstalled.count();
    for (const auto& lib : mOnlineLibs) {
      if (toBeInstalled.contains(lib.uuid)) {
        toBeInstalled |= lib.dependencies;
      }
    }
    if (toBeInstalled.count() == oldCount) {
      break;
    }
  }

  for (const Uuid& uuid : toBeInstalled) {
    mCheckStates[uuid] = true;
  }
}

void LibrariesModel::uncheckLibsWithUnmetDependencies() noexcept {
  QSet<Uuid> toBeUninstalled;
  for (const auto& lib : mMergedLibs) {
    const auto uuid = Uuid::tryFromString(s2q(lib.uuid));
    if (uuid && (!isLibraryChecked(lib))) {
      toBeUninstalled.insert(*uuid);
    }
  }

  while (true) {
    const int oldCount = toBeUninstalled.count();
    for (const auto& lib : mOnlineLibs) {
      if (!(lib.dependencies & toBeUninstalled).isEmpty()) {
        toBeUninstalled.insert(lib.uuid);
      }
    }
    if (toBeUninstalled.count() == oldCount) {
      break;
    }
  }

  for (const Uuid& uuid : toBeUninstalled) {
    mCheckStates[uuid] = false;
  }
}

bool LibrariesModel::isLibraryChecked(
    const ui::LibraryInfoData& lib) const noexcept {
  const auto uuid = Uuid::tryFromString(s2q(lib.uuid));
  return uuid && mCheckStates.value(*uuid, !lib.installed_version.empty());
}

bool LibrariesModel::isMarkedForInstall(
    const ui::LibraryInfoData& lib) noexcept {
  return lib.installed_version.empty() && lib.checked;
}

bool LibrariesModel::isMarkedForUpdate(
    const ui::LibraryInfoData& lib) noexcept {
  return (!lib.installed_version.empty()) && lib.outdated && lib.checked;
}

bool LibrariesModel::isMarkedForUninstall(
    const ui::LibraryInfoData& lib) noexcept {
  return (!lib.installed_version.empty()) && (!lib.checked);
}

std::optional<std::size_t> LibrariesModel::indexOf(const Uuid& uuid) noexcept {
  for (std::size_t i = 0; i < mMergedLibs.size(); ++i) {
    if (mMergedLibs.at(i).uuid == uuid.toStr()) {
      return i;
    }
  }
  return std::nullopt;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
