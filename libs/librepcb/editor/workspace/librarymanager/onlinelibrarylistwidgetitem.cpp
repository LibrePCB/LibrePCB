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
#include "onlinelibrarylistwidgetitem.h"

#include "librarydownload.h"
#include "ui_onlinelibrarylistwidgetitem.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/network/networkrequest.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

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

OnlineLibraryListWidgetItem::OnlineLibraryListWidgetItem(
    Workspace& ws, const ApiEndpoint::Library& obj) noexcept
  : QWidget(nullptr),
    mWorkspace(ws),
    mLibrary(obj),
    mUi(new Ui::OnlineLibraryListWidgetItem),
    mAutoCheck(true) {
  mUi->setupUi(this);
  mUi->lblIcon->setText("");
  mUi->prgProgress->setVisible(false);
  connect(mUi->cbxDownload, &QCheckBox::toggled, this,
          &OnlineLibraryListWidgetItem::checkedChanged);
  connect(mUi->cbxDownload, &QCheckBox::clicked, this,
          [this]() { mAutoCheck = false; });

  mUi->lblName->setText(
      QString("%1 v%2").arg(mLibrary.name, mLibrary.version.toStr()));

  // Only show the first line to avoid breaking the UI layout.
  mUi->lblDescription->setText(mLibrary.description.split("\n").first());
  mUi->lblAuthor->setText(QString("Author: %1").arg(mLibrary.author));

  NetworkRequest* request = new NetworkRequest(mLibrary.iconUrl);
  request->setMinimumCacheTime(24 * 3600);  // 1 day
  connect(request, &NetworkRequest::dataReceived, this,
          &OnlineLibraryListWidgetItem::iconReceived, Qt::QueuedConnection);
  request->start();

  // check if this library is already installed
  updateInstalledStatus();
  connect(&mWorkspace.getLibraryDb(),
          &WorkspaceLibraryDb::scanLibraryListUpdated, this,
          &OnlineLibraryListWidgetItem::updateInstalledStatus);
}

OnlineLibraryListWidgetItem::~OnlineLibraryListWidgetItem() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool OnlineLibraryListWidgetItem::isChecked() const noexcept {
  return mUi->cbxDownload->isChecked();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void OnlineLibraryListWidgetItem::setChecked(bool checked) noexcept {
  mUi->cbxDownload->setChecked(checked);
  mAutoCheck = false;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void OnlineLibraryListWidgetItem::startDownloadIfSelected() noexcept {
  if (mUi->cbxDownload->isVisible() && mUi->cbxDownload->isChecked() &&
      (!mLibraryDownload)) {
    mUi->cbxDownload->setVisible(false);
    mUi->prgProgress->setVisible(true);

    // determine destination directory
    QString libDirName = mLibrary.uuid.toStr() % ".lplib";
    FilePath destDir =
        mWorkspace.getLibrariesPath().getPathTo("remote/" % libDirName);

    // start download
    mLibraryDownload.reset(new LibraryDownload(mLibrary.downloadUrl, destDir));
    if (mLibrary.downloadSize > 0) {
      mLibraryDownload->setExpectedZipFileSize(mLibrary.downloadSize);
    }
    if (!mLibrary.downloadSha256.isEmpty()) {
      mLibraryDownload->setExpectedChecksum(
          QCryptographicHash::Sha256,
          QByteArray::fromHex(mLibrary.downloadSha256));
    }
    connect(mLibraryDownload.data(), &LibraryDownload::progressPercent,
            mUi->prgProgress, &QProgressBar::setValue, Qt::QueuedConnection);
    connect(mLibraryDownload.data(), &LibraryDownload::finished, this,
            &OnlineLibraryListWidgetItem::downloadFinished,
            Qt::QueuedConnection);
    mLibraryDownload->start();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void OnlineLibraryListWidgetItem::downloadFinished(
    bool success, const QString& errMsg) noexcept {
  Q_ASSERT(mLibraryDownload);

  if ((!success) && (!errMsg.isEmpty())) {
    QMessageBox::critical(this, tr("Download failed"), errMsg);
  }

  // Hide the progress bar as the download is finished, but don't update the
  // other widgets because the database has not yet indexed the new library!
  // The method updateInstalledStatus() will be called automatically once the
  // new library is indexed.
  mUi->prgProgress->setVisible(false);

  // delete download helper
  mLibraryDownload.reset();

  // start library scanner to index the new library
  mWorkspace.getLibraryDb().startLibraryRescan();
}

void OnlineLibraryListWidgetItem::iconReceived(
    const QByteArray& data) noexcept {
  QPixmap pixmap;
  pixmap.loadFromData(data);
  mUi->lblIcon->setPixmap(pixmap);
}

void OnlineLibraryListWidgetItem::updateInstalledStatus() noexcept {
  // Don't update the widgets while the download is running, it would mess up
  // the UI!
  if (mLibraryDownload) {
    return;
  }

  tl::optional<Version> installedVersion;
  try {
    FilePath fp = mWorkspace.getLibraryDb().getLatest<Library>(
        mLibrary.uuid);  // can throw
    if (fp.isValid()) {
      Version v = Version::fromString("0.1");  // only for initialization
      mWorkspace.getLibraryDb().getMetadata<Library>(fp, nullptr,
                                                     &v);  // can throw
      installedVersion = v;
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to determine if library is installed: "
                << e.getMsg();
  }
  if (installedVersion) {
    if (installedVersion < mLibrary.version) {
      mUi->lblInstalledVersion->setText(
          tr("v%1").arg(installedVersion->toStr()));
      mUi->lblInstalledVersion->setStyleSheet("QLabel {color: red;}");
      mUi->cbxDownload->setText(tr("Update") % ":");
      mUi->cbxDownload->setVisible(true);
    } else {
      mUi->lblInstalledVersion->setText(tr("Installed"));
      mUi->lblInstalledVersion->setStyleSheet("QLabel {color: green;}");
      mUi->cbxDownload->setVisible(false);
    }
    mUi->lblInstalledVersion->setVisible(true);
  } else {
    if (mLibrary.recommended) {
      mUi->lblInstalledVersion->setText(tr("Recommended"));
      mUi->lblInstalledVersion->setStyleSheet("QLabel {color: blue;}");
      mUi->lblInstalledVersion->setVisible(true);
    } else {
      mUi->lblInstalledVersion->setVisible(false);
    }
    mUi->cbxDownload->setText(tr("Install") % ":");
    mUi->cbxDownload->setVisible(true);
  }
  if (mAutoCheck) {
    mUi->cbxDownload->setChecked(installedVersion
                                     ? (installedVersion < mLibrary.version)
                                     : mLibrary.recommended);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
