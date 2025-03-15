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

#include "../utils/editortoolbox.h"

#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/network/apiendpoint.h>
#include <librepcb/core/network/networkrequest.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/library/libraryeditor.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

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

int LibrariesModel::getOutdatedLibraries() const noexcept {
  int count = 0;
  for (const auto& lib : mMergedLibs) {
    if (lib.state == ui::LibraryState::Outdated) {
      ++count;
    }
  }
  return count;
}

int LibrariesModel::getCheckedLibraries() const noexcept {
  int count = 0;
  for (const auto& lib : mMergedLibs) {
    if (lib.checked && (!lib.filtered_out)) {
      ++count;
    }
  }
  return count;
}

void LibrariesModel::installCheckedLibraries() noexcept {
  for (auto& lib : mMergedLibs) {
    if ((!lib.checked) || (lib.filtered_out)) continue;

    auto uuid = Uuid::tryFromString(s2q(lib.id));
    if (!uuid) continue;

    const auto remoteLib = mRemoteLibs.find(*uuid);
    if (remoteLib == mRemoteLibs.end()) continue;

    // Determine destination directory.
    const FilePath destDir = mWorkspace.getLibrariesPath().getPathTo(
        "remote/" % remoteLib->uuid.toStr() % ".lplib");

    // Start download.
    auto dl =
        std::make_shared<LibraryDownload>(remoteLib->downloadUrl, destDir);
    if (remoteLib->downloadSize > 0) {
      dl->setExpectedZipFileSize(remoteLib->downloadSize);
    }
    if (!remoteLib->downloadSha256.isEmpty()) {
      dl->setExpectedChecksum(QCryptographicHash::Sha256,
                              QByteArray::fromHex(remoteLib->downloadSha256));
    }
    connect(
        dl.get(), &LibraryDownload::progressPercent, this,
        [this, uuid](int percent) {
          if (auto i = findLib(uuid->toStr())) {
            mMergedLibs.at(*i).progress = percent;
            row_changed(*i);
          }
        },
        Qt::QueuedConnection);
    connect(
        dl.get(), &LibraryDownload::finished, this,
        [this, uuid, dl](bool success, const QString& errMsg) {
          Q_UNUSED(success);
          Q_UNUSED(errMsg);
          if (auto i = findLib(uuid->toStr())) {
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
}

void LibrariesModel::openLibrary(const slint::SharedString& id) noexcept {
  try {
    auto libEditor = new LibraryEditor(mWorkspace, FilePath(s2q(id)), false);
    libEditor->show();
  } catch (const Exception& e) {
    // TODO
  }
}

void LibrariesModel::uninstallLibrary(const slint::SharedString& id) noexcept {
  try {
    FileUtils::removeDirRecursively(FilePath(s2q(id)));  // can throw
  } catch (const Exception& e) {
    // TODO
  }
  mWorkspace.getLibraryDb().startLibraryRescan();
}

void LibrariesModel::toggleAll(bool checked) noexcept {
  for (std::size_t i = 0; i < mMergedLibs.size(); ++i) {
    mMergedLibs[i].checked = checked;
    row_changed(i);
  }
}

void LibrariesModel::clearFilter() noexcept {
  if (!mFilterTerm.isEmpty()) {
    mFilterTerm.clear();
    emit filterTermChanged(mFilterTerm);
    applyFilter();
  }
}

slint::private_api::EventResult LibrariesModel::keyEvent(
    const slint::private_api::KeyEvent& e) noexcept {
  if (e.event_type != slint::private_api::KeyEventType::KeyPressed) {
    return slint::private_api::EventResult::Reject;
  }

  const QString text(s2q(e.text));
  if (text.size() != 1) {
    return slint::private_api::EventResult::Reject;
  }
  const QChar c = text.front();

  if ((c == '\x1b') && (mFilterTerm.size() > 0)) {
    mFilterTerm.clear();
    emit filterTermChanged(mFilterTerm);
    applyFilter();
    return slint::private_api::EventResult::Accept;
  } else if ((c == '\b') && (mFilterTerm.size() > 0)) {
    mFilterTerm.chop(1);
    emit filterTermChanged(mFilterTerm);
    applyFilter();
    return slint::private_api::EventResult::Accept;
  } else if (c.isPrint()) {
    mFilterTerm += c;
    emit filterTermChanged(mFilterTerm);
    applyFilter();
    return slint::private_api::EventResult::Accept;
  } else {
    return slint::private_api::EventResult::Reject;
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t LibrariesModel::row_count() const {
  return mMergedLibs.size();
}

std::optional<ui::LibraryData> LibrariesModel::row_data(std::size_t i) const {
  return (i < mMergedLibs.size()) ? std::optional(mMergedLibs.at(i))
                                  : std::nullopt;
}

void LibrariesModel::set_row_data(std::size_t i,
                                  const ui::LibraryData& obj) noexcept {
  if (i < mMergedLibs.size()) {
    mMergedLibs.at(i) = obj;
    row_changed(i);
  }
  emit checkedLibrariesChanged(getCheckedLibraries());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibrariesModel::refreshLocalLibraries() noexcept {
  mLocalLibs.clear();
  mLocalLibsErrors.clear();

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

      const auto sName = q2s(name);
      mLocalLibs.insert(
          uuid,
          ui::LibraryData{
              q2s(libDir.toStr()),
              sName,
              q2s(description),
              q2s(version.toStr()),
              q2s(icon),
              false,
              isRemote ? ui::LibraryType::Remote : ui::LibraryType::Local,
              ui::LibraryState::Unknown,
              0,
              false,
              filterOut(sName),
          });
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to update library list:" << e.getMsg();
    mLocalLibsErrors.append(e.getMsg());
  }

  refreshMergedLibs();

  emit errorsChanged(getErrors());
}

void LibrariesModel::refreshRemoteLibraries() noexcept {
  mApiEndpointsInProgress.clear();  // disconnects all signal/slot connections
  mRemoteLibs.clear();
  mRemoteLibsErrors.clear();
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
    fetchingRemoteLibrariesChanged(true);
  }
}

void LibrariesModel::onlineLibraryListReceived(
    QList<ApiEndpoint::Library> libs) noexcept {
  for (const auto& lib : libs) {
    mRemoteLibs.insert(lib.uuid, lib);
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

  apiEndpointOperationFinished();
}

void LibrariesModel::errorWhileFetchingLibraryList(QString errorMsg) noexcept {
  const ApiEndpoint* endpoint = qobject_cast<ApiEndpoint*>(sender());
  mRemoteLibsErrors.append(
      tr("Error while fetching libraries from '%1': %2")
          .arg(endpoint ? endpoint->getUrl().toString() : QString())
          .arg(errorMsg));
  emit errorsChanged(getErrors());

  apiEndpointOperationFinished();
}

void LibrariesModel::apiEndpointOperationFinished() noexcept {
  const ApiEndpoint* endpoint = qobject_cast<ApiEndpoint*>(sender());
  mApiEndpointsInProgress.removeIf([endpoint](std::shared_ptr<ApiEndpoint> ep) {
    return ep.get() == endpoint;
  });
  if (mApiEndpointsInProgress.isEmpty()) {
    fetchingRemoteLibrariesChanged(false);
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
      it->checked = (it->state == ui::LibraryState::Outdated);
    } else {
      auto name = q2s(lib.name);
      mMergedLibs.push_back(ui::LibraryData{
          q2s(lib.uuid.toStr()),
          name,
          q2s(lib.description),
          q2s(lib.version.toStr()),
          q2s(mRemoteIcons.value(lib.uuid)),
          lib.recommended,
          ui::LibraryType::Online,
          ui::LibraryState::Unknown,
          0,
          lib.recommended,
          filterOut(name),
      });
    }
  }
  for (const auto& lib : mLocalLibs) {
    mMergedLibs.push_back(lib);
  }
  std::sort(mMergedLibs.begin(), mMergedLibs.end(),
            [](const ui::LibraryData& a, const ui::LibraryData& b) {
              if ((a.state == ui::LibraryState::Outdated) !=
                  (b.state == ui::LibraryState::Outdated)) {
                return a.state == ui::LibraryState::Outdated;
              } else if (a.type != b.type) {
                return static_cast<int>(a.type) < static_cast<int>(b.type);
              } else if (a.recommended != b.recommended) {
                return a.recommended;
              } else {
                return a.name < b.name;
              }
            });
  reset();
  emit outdatedLibrariesChanged(getOutdatedLibraries());
  emit checkedLibrariesChanged(getCheckedLibraries());
}

void LibrariesModel::applyFilter() noexcept {
  bool modified = false;
  for (std::size_t i = 0; i < mMergedLibs.size(); ++i) {
    const bool filteredOut = filterOut(mMergedLibs.at(i).name);
    if (mMergedLibs.at(i).filtered_out != filteredOut) {
      mMergedLibs.at(i).filtered_out = filteredOut;
      row_changed(i);
      modified = true;
    }
  }

  if (modified) {
    emit checkedLibrariesChanged(getCheckedLibraries());
  }
}

bool LibrariesModel::filterOut(const slint::SharedString& name) const noexcept {
  return (!mFilterTerm.isEmpty()) &&
      !s2q(name).toLower().contains(mFilterTerm.toLower());
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

}  // namespace editor
}  // namespace librepcb
