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

#include "../notification.h"
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

static AutoUpdateMode getUpdateMode(const Workspace& ws) noexcept {
  return ws.getSettings().librariesAutoUpdateMode.get();
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibrariesModel::LibrariesModel(Workspace& ws, Mode mode,
                               QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mMode(mode),
    mInitialized(false),
    mIsAutoCheck(false),
    mAutoUpdateErrorCount(0),
    mRequestIcons(false) {
  connect(&mWorkspace.getLibraryDb(),
          &WorkspaceLibraryDb::scanLibraryListUpdated, this,
          &LibrariesModel::updateLibraries, Qt::QueuedConnection);

  // Setup auto-update.
  connect(&mAutoUpdateTimer, &QTimer::timeout, this, [this]() {
    mAutoUpdateTimer.setInterval(25 * 3600 * 1000);  // 25h
    ensurePopulated(false, true);
  });
  mAutoUpdateTimer.start(1000);

  // When the list of API endpoints is modified, re-fetch all remote libraries.
  if (mMode == Mode::RemoteLibs) {
    connect(&mWorkspace.getSettings().apiEndpoints,
            &WorkspaceSettingsItem::edited, this, [this]() {
              mApiEndpointsInProgress.clear();
              mOnlineLibsErrors.clear();
              mOnlineLibs.clear();
              updateMergedLibraries();
              ensurePopulated(false, false);
            });
  }

  // Load auto-update error count.
  if (mMode == Mode::RemoteLibs) {
    QSettings cs;
    mAutoUpdateErrorCount =
        cs.value("library_manager/auto_update_errors", 0).toInt();
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
    if (lib.duplicate) {
      ++data.pending_uninstalls;
      continue;
    }
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

void LibrariesModel::ensurePopulated(bool withIcons,
                                     bool isAutoCheck) noexcept {
  if (withIcons) {
    mRequestIcons = true;
  }
  mIsAutoCheck = isAutoCheck;
  if (mInstalledLibs.empty() || (!mInstalledLibsErrors.isEmpty())) {
    updateLibraries(false);
  }
  if ((mMode == Mode::RemoteLibs) &&
      (getUpdateMode(mWorkspace) != AutoUpdateMode::Disabled) &&
      (mApiEndpointsInProgress.isEmpty() &&
       (mOnlineLibs.isEmpty() || (!mOnlineLibsErrors.isEmpty())))) {
    requestOnlineLibraries(false);
  }
  if (mRequestIcons) {
    requestMissingOnlineIcons();
  }
}

void LibrariesModel::highlightLibraryOnNextRescan(const FilePath& fp) noexcept {
  mHighlightedLib = fp;
}

void LibrariesModel::checkForUpdates() noexcept {
  if (mMode != Mode::RemoteLibs) return;

  mIsAutoCheck = false;
  requestOnlineLibraries(true);
}

void LibrariesModel::cancelUpdateCheck() noexcept {
  if (mMode != Mode::RemoteLibs) return;

  mApiEndpointsInProgress.clear();  // Should cancel the network requests.
  mOnlineLibsErrors.clear();
  emit uiDataChanged(getUiData());
}

void LibrariesModel::toggleAll() noexcept {
  if (mMergedLibs.empty()) return;

  // Determine new check state depending on how many libs are checked.
  std::size_t checkedLibs = 0;
  for (const auto& item : mMergedLibs) {
    if (item.checked) {
      ++checkedLibs;
    }
  }
  const bool newCheckState = checkedLibs <= (mMergedLibs.size() / 2);

  // Apply new check state to all libs.
  for (const auto& item : mMergedLibs) {
    if (auto uuid = Uuid::tryFromString(s2q(item.uuid))) {
      mCheckStates[*uuid] = newCheckState;
    }
  }
  if (newCheckState) {
    checkMissingDependenciesOfLibs();
  } else {
    uncheckLibsWithUnmetDependencies();
  }
  updateCheckStates(true);
  emit uiDataChanged(getUiData());
}

void LibrariesModel::applyChanges() noexcept {
  if (mMode != Mode::RemoteLibs) return;

  // Abort any currently running library rescan since this can cause problems
  // due to concurrent file access. The scan will be restarted automatically
  // after all changes have been applied (i.e. libraries removed or installed).
  const bool scanCanceled = mWorkspace.getLibraryDb().cancelLibraryRescan();

  // Show wait cursor since some operations can take a while.
  qApp->setOverrideCursor(Qt::WaitCursor);
  auto cursorSg = scopeGuard([]() { qApp->restoreOverrideCursor(); });

  // Limit number of parallel file access threads, mainly for unreliable
  // operating systems like MS Windows. It seems to be overwhelmed when
  // extracting more than a few ZIP files at the same time...
  auto semaphore = std::make_shared<QSemaphore>(4);

  // Remove libraries first.
  int uninstalled = 0;
  for (auto& lib : mMergedLibs) {
    const auto uuid = Uuid::tryFromString(s2q(lib.uuid));
    if ((!uuid) || (!isMarkedForUninstall(lib))) continue;

    try {
      const FilePath fp(s2q(lib.path));
      emit aboutToUninstallLibrary(fp);  // Let the app close it if needed
      FileUtils::removeDirRecursively(fp);  // can throw
      mInstalledLibDirs[*uuid].remove(fp);  // Important for duplicates!
    } catch (const Exception& e) {
      emit notificationEmitted(std::make_shared<Notification>(
          ui::NotificationType::Critical, tr("Failed to Uninstall Library"),
          tr("The directory '%1' could not be removed: %2")
              .arg(s2q(lib.path), e.getMsg()),
          QString(), QString(), true));
    }
    ++uninstalled;
  }

  // Then start installations/updates.
  int installed = 0;
  for (auto& lib : mMergedLibs) {
    const auto uuid = Uuid::tryFromString(s2q(lib.uuid));
    if ((!uuid) || ((!isMarkedForInstall(lib)) && (!isMarkedForUpdate(lib)))) {
      continue;
    }

    // Install/update library.
    const auto onlineLib = mOnlineLibs.find(*uuid);
    if (onlineLib == mOnlineLibs.end()) continue;

    // Determine destination directory.
    const FilePath destDir = mWorkspace.getLibrariesPath().getPathTo(
        "remote/" % onlineLib->uuid.toStr() % ".lplib");

    // Start download.
    auto dl = std::make_shared<LibraryDownload>(onlineLib->downloadUrl, destDir,
                                                semaphore);
    if (onlineLib->downloadSize > 0) {
      dl->setExpectedZipFileSize(onlineLib->downloadSize);
    }
    if (!onlineLib->downloadSha256.isEmpty()) {
      dl->setExpectedChecksum(QCryptographicHash::Sha256,
                              QByteArray::fromHex(onlineLib->downloadSha256));
    }
    dl->setExistingDirsToReplace(mInstalledLibDirs[onlineLib->uuid]);
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
  }

  if ((installed == 0) && ((uninstalled > 0) || scanCanceled)) {
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
  mInstalledLibDirs.clear();

  try {
    QSet<Uuid> uuids;
    QMultiMap<Version, FilePath> libraries =
        mWorkspace.getLibraryDb().getAll<Library>();  // can throw

    // Note: Iterate from high versions to low versions, to make sure the
    // old versions are marked as duplicate, not the new versions. This avoids
    // updating duplicate libraries even though another duplicate is already
    // up-to-date.
    auto rbegin = std::make_reverse_iterator(libraries.constEnd());
    auto rend = std::make_reverse_iterator(libraries.constBegin());
    for (auto it = rbegin; it != rend; ++it) {
      const FilePath libDir = *it;
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
          q2s(uuid.toStr()),  // UUID
          q2s(libDir.toNative()),  // Path
          q2s(icon),  // Icon
          sName,  // Name
          q2s(description),  // Description
          q2s(version.toStr()),  // Installed version
          slint::SharedString(),  // Online version
          false,  // Outdated
          uuids.contains(uuid),  // Duplicate
          false,  // Recommended
          0,  // Progress [%]
          !uuids.contains(uuid),  // Checked
          mHighlightedLib == libDir,  // Highlight
      });
      mInstalledLibDirs[uuid].insert(libDir);
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

void LibrariesModel::requestOnlineLibraries(bool forceNoCache) noexcept {
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
      repo->requestLibraryList(forceNoCache);
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
  updateMergedLibraries();
  emit onlineVersionsAvailable(versions);

  // Must come after updateMergedLibraries(), for the auto-update.
  apiEndpointOperationFinished();

  if (mRequestIcons) {
    requestMissingOnlineIcons();
  }
}

void LibrariesModel::requestMissingOnlineIcons() noexcept {
  for (const ApiEndpoint::Library& lib : std::as_const(mOnlineLibs)) {
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
          .arg(endpoint ? endpoint->getUrl().toString() : QString(), errorMsg));
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
    const auto uiData = getUiData();
    emit uiDataChanged(uiData);

    // IMPORTANT: Depend auto-update on "pending_updates", not on "outdated"!
    // If a library is installed multiple times, "outdated" stays > 0 even
    // after the auto-update, so every app restart would re-trigger the update.
    qInfo() << "Remote libraries with update available:"
            << uiData.pending_updates;
    if (mIsAutoCheck && (uiData.pending_updates > 0) &&
        mDownloadsInProgress.isEmpty()) {
      AutoUpdateMode mode = getUpdateMode(mWorkspace);
      if (mode == AutoUpdateMode::Install) {
        if (mAutoUpdateErrorCount < 3) {
          setAutoUpdateErrorCount(mAutoUpdateErrorCount + 1);
          startAutoUpdate();
        } else {
          mode = AutoUpdateMode::Notify;
          qCritical() << "Automatic library update not executed due to too "
                         "many previous errors.";
        }
      }
      if (mode == AutoUpdateMode::Notify) {
        auto notification = std::make_shared<Notification>(
            ui::NotificationType::Info, tr("Library Updates Available"),
            tr("There are %n library update(s) available for installation. See "
               "details in the libraries side panel, or click the button below "
               "to download & install all updates.",
               nullptr, uiData.pending_updates),
            tr("Update Libraries"), QString(), true);
        connect(notification.get(), &Notification::buttonClicked, this,
                &LibrariesModel::startAutoUpdate, Qt::QueuedConnection);
        connect(notification.get(), &Notification::buttonClicked,
                notification.get(), &Notification::dismiss,
                Qt::QueuedConnection);
        connect(this, &LibrariesModel::uiDataChanged, notification.get(),
                &Notification::dismiss, Qt::QueuedConnection);
        emit notificationEmitted(notification);
      }
    }
  }
}

void LibrariesModel::startAutoUpdate() noexcept {
  const auto uiData = getUiData();
  qInfo() << "Starting update of" << uiData.pending_updates
          << "remote libraries...";
  emit statusBarMessageChanged(
      tr("Updating %n libraries...", nullptr, uiData.pending_updates), 4000);
  mCheckStates.clear();
  updateMergedLibraries();
  applyChanges();
}

void LibrariesModel::updateMergedLibraries() noexcept {
  mMergedLibs = mInstalledLibs;
  for (auto& lib : mMergedLibs) {
    lib.online_version = mOnlineVersions.value(lib.uuid);
  }

  for (const auto& lib : std::as_const(mOnlineLibs)) {
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
          false,  // Duplicate
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

  const ui::LibraryListData uiData = getUiData();
  if ((mMode == Mode::RemoteLibs) && uiData.all_up_to_date) {
    setAutoUpdateErrorCount(0);
  }
  emit uiDataChanged(uiData);
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
  for (const auto& lib : std::as_const(mMergedLibs)) {
    const auto uuid = Uuid::tryFromString(s2q(lib.uuid));
    if (uuid && isLibraryChecked(lib)) {
      toBeInstalled.insert(*uuid);
    }
  }

  while (true) {
    const int oldCount = toBeInstalled.count();
    for (const auto& lib : std::as_const(mOnlineLibs)) {
      if (toBeInstalled.contains(lib.uuid)) {
        toBeInstalled |= lib.dependencies;
      }
    }
    if (toBeInstalled.count() == oldCount) {
      break;
    }
  }

  for (const Uuid& uuid : std::as_const(toBeInstalled)) {
    mCheckStates[uuid] = true;
  }
}

void LibrariesModel::uncheckLibsWithUnmetDependencies() noexcept {
  QSet<Uuid> toBeUninstalled;
  for (const auto& lib : std::as_const(mMergedLibs)) {
    const auto uuid = Uuid::tryFromString(s2q(lib.uuid));
    if (uuid && (!isLibraryChecked(lib))) {
      toBeUninstalled.insert(*uuid);
    }
  }

  while (true) {
    const int oldCount = toBeUninstalled.count();
    for (const auto& lib : std::as_const(mOnlineLibs)) {
      if (!(lib.dependencies & toBeUninstalled).isEmpty()) {
        toBeUninstalled.insert(lib.uuid);
      }
    }
    if (toBeUninstalled.count() == oldCount) {
      break;
    }
  }

  for (const Uuid& uuid : std::as_const(toBeUninstalled)) {
    mCheckStates[uuid] = false;
  }
}

bool LibrariesModel::isLibraryChecked(
    const ui::LibraryInfoData& lib) const noexcept {
  const auto uuid = Uuid::tryFromString(s2q(lib.uuid));
  return uuid && (!lib.duplicate) &&
      mCheckStates.value(*uuid, !lib.installed_version.empty());
}

bool LibrariesModel::isMarkedForInstall(
    const ui::LibraryInfoData& lib) noexcept {
  return lib.installed_version.empty() && lib.checked && (!lib.duplicate);
}

bool LibrariesModel::isMarkedForUpdate(
    const ui::LibraryInfoData& lib) noexcept {
  return (!lib.installed_version.empty()) && lib.outdated && lib.checked &&
      (!lib.duplicate);
}

bool LibrariesModel::isMarkedForUninstall(
    const ui::LibraryInfoData& lib) noexcept {
  return ((!lib.installed_version.empty()) && (!lib.checked)) || lib.duplicate;
}

std::optional<std::size_t> LibrariesModel::indexOf(const Uuid& uuid) noexcept {
  for (std::size_t i = 0; i < mMergedLibs.size(); ++i) {
    if (mMergedLibs.at(i).uuid == uuid.toStr()) {
      return i;
    }
  }
  return std::nullopt;
}

void LibrariesModel::setAutoUpdateErrorCount(int count) noexcept {
  if (count != mAutoUpdateErrorCount) {
    QSettings cs;
    cs.setValue("library_manager/auto_update_errors", count);
    mAutoUpdateErrorCount = count;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
