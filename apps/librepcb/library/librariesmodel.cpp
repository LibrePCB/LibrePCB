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

#include "../apptoolbox.h"

#include <librepcb/core/network/apiendpoint.h>
#include <librepcb/core/network/networkrequest.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

#include <optional>

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
  connect(&mWorkspace.getLibraryDb(),
          &WorkspaceLibraryDb::scanLibraryListUpdated, this,
          &LibrariesModel::refreshLocalLibraries, Qt::QueuedConnection);
}

LibrariesModel::~LibrariesModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void LibrariesModel::ensurePopulated() noexcept {
  refreshLocalLibraries();
  refreshRemoteLibraries();
}

void LibrariesModel::installLibrary(const slint::SharedString& id) noexcept {
  // Determine library.
  std::optional<ApiEndpoint::Library> lib;
  for (const auto& l : mRemoteLibs) {
    if (l.uuid.toStr() == id) {
      lib = l;
      break;
    }
  }
  if (!lib) {
    qCritical() << "Failed to determine library!";
    return;
  }

  // Determine destination directory.
  const FilePath destDir = mWorkspace.getLibrariesPath().getPathTo(
      "remote/" % lib->uuid.toStr() % ".lplib");

  // Start download.
  auto dl = std::make_shared<LibraryDownload>(lib->downloadUrl, destDir);
  if (lib->downloadSize > 0) {
    dl->setExpectedZipFileSize(lib->downloadSize);
  }
  if (!lib->downloadSha256.isEmpty()) {
    dl->setExpectedChecksum(QCryptographicHash::Sha256,
                            QByteArray::fromHex(lib->downloadSha256));
  }
  const Uuid uuid = lib->uuid;
  connect(
      dl.get(), &LibraryDownload::progressPercent, this,
      [this, uuid](int percent) {
        if (auto i = findLib(uuid.toStr())) {
          mMergedLibs.at(*i).progress = percent;
          row_changed(*i);
        }
      },
      Qt::QueuedConnection);
  connect(
      dl.get(), &LibraryDownload::finished, this,
      [this, uuid, dl](bool success, const QString& errMsg) {
        if (auto i = findLib(uuid.toStr())) {
          mMergedLibs.at(*i).progress = 0;
          row_changed(*i);
        }
        mDownloadsInProgress.removeOne(dl);
        mWorkspace.getLibraryDb().startLibraryRescan();
      },
      Qt::QueuedConnection);
  mDownloadsInProgress.append(dl);
  dl->start();
}

void LibrariesModel::uninstallLibrary(const slint::SharedString& id) noexcept {
  qDebug() << "Uninstall" << id.data();
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t LibrariesModel::row_count() const {
  return mMergedLibs.size();
}

std::optional<ui::Library> LibrariesModel::row_data(std::size_t i) const {
  return (i < mMergedLibs.size()) ? std::optional(mMergedLibs.at(i))
                                  : std::nullopt;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibrariesModel::refreshLocalLibraries() noexcept {
  mLocalLibs.clear();

  try {
    QMultiMap<Version, FilePath> libraries =
        mWorkspace.getLibraryDb().getAll<Library>();  // can throw

    foreach (const FilePath& libDir, libraries) {
      Uuid uuid = Uuid::createRandom();
      Version version = Version::fromString("1");
      mWorkspace.getLibraryDb().getMetadata<Library>(libDir, &uuid,
                                                     &version);  // can throw
      QString name, description, keywords;
      mWorkspace.getLibraryDb().getTranslations<Library>(
          libDir, mWorkspace.getSettings().libraryLocaleOrder.get(), &name,
          &description, &keywords);  // can throw
      QPixmap icon;
      mWorkspace.getLibraryDb().getLibraryMetadata(libDir, &icon);  // can throw
      const bool isRemote =
          libDir.isLocatedInDir(mWorkspace.getRemoteLibrariesPath());

      mLocalLibs.insert(
          uuid,
          ui::Library{
              q2s(libDir.toStr()),
              q2s(name),
              q2s(description),
              q2s(version.toStr()),
              q2s(icon),
              isRemote ? ui::LibraryType::Remote : ui::LibraryType::Local,
              ui::LibraryState::Unknown,
              0,
          });
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to update library list:" << e.getMsg();
  }

  refreshMergedLibs();
}

void LibrariesModel::refreshRemoteLibraries() noexcept {
  mApiEndpointsInProgress.clear();  // disconnects all signal/slot connections
  mRemoteLibs.clear();
  foreach (const QUrl& url, mWorkspace.getSettings().apiEndpoints.get()) {
    std::shared_ptr<ApiEndpoint> repo = std::make_shared<ApiEndpoint>(url);
    connect(repo.get(), &ApiEndpoint::libraryListReceived, this,
            &LibrariesModel::onlineLibraryListReceived);
    connect(repo.get(), &ApiEndpoint::errorWhileFetchingLibraryList, this,
            &LibrariesModel::errorWhileFetchingLibraryList);
    mApiEndpointsInProgress.append(repo);
    repo->requestLibraryList();
  }
  if (!mApiEndpointsInProgress.isEmpty()) {
    remoteLibrariesFetchingChanged(true);
  }
}

void LibrariesModel::onlineLibraryListReceived(
    QList<ApiEndpoint::Library> libs) noexcept {
  mRemoteLibs.append(libs);
  for (const auto& lib : libs) {
    const Uuid uuid = lib.uuid;
    if (!mRemoteIcons.contains(uuid)) {
      NetworkRequest* request = new NetworkRequest(lib.iconUrl);
      request->setMinimumCacheTime(24 * 3600);  // 1 day
      connect(
          request, &NetworkRequest::dataReceived, this,
          [this, uuid](const QByteArray& data) {
            QPixmap pixmap;
            pixmap.loadFromData(data);
            mRemoteIcons[uuid] = pixmap;
            for (std::size_t i = 0; i < mMergedLibs.size(); ++i) {
              if (mMergedLibs.at(i).id == uuid.toStr()) {
                mMergedLibs.at(i).icon = q2s(pixmap);
                row_changed(i);
              }
            }
          },
          Qt::QueuedConnection);
      request->start();
    }
  }
  refreshMergedLibs();
  remoteLibrariesFetchingChanged(false);
}

void LibrariesModel::errorWhileFetchingLibraryList(QString errorMsg) noexcept {
  Q_UNUSED(errorMsg);
  if (mApiEndpointsInProgress.isEmpty()) {
    remoteLibrariesFetchingChanged(false);
  }
}

void LibrariesModel::refreshMergedLibs() noexcept {
  mMergedLibs.clear();
  for (const auto& lib : mRemoteLibs) {
    auto it = mLocalLibs.find(lib.uuid);
    if (it != mLocalLibs.end()) {
      it->state = (Version::fromString(it->version.data()) >= lib.version)
          ? ui::LibraryState::UpToDate
          : ui::LibraryState::Outdated;
    } else {
      mMergedLibs.push_back(ui::Library{
          q2s(lib.uuid.toStr()),
          q2s(lib.name),
          q2s(lib.description),
          q2s(lib.version.toStr()),
          q2s(mRemoteIcons.value(lib.uuid)),
          ui::LibraryType::Online,
          ui::LibraryState::Unknown,
          0,
      });
    }
  }
  for (const auto& lib : mLocalLibs) {
    mMergedLibs.push_back(lib);
  }
  std::sort(mMergedLibs.begin(), mMergedLibs.end(),
            [](const ui::Library& a, const ui::Library& b) {
              return a.name < b.name;
            });
  reset();
}

std::optional<std::size_t> LibrariesModel::findLib(const QString& id) noexcept {
  for (std::size_t i = 0; i < mMergedLibs.size(); ++i) {
    if (mMergedLibs.at(i).id == id) {
      return i;
    }
  }
  return std::nullopt;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
