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
#include "repositorylibrarylistwidgetitem.h"

#include "librarydownload.h"
#include "ui_repositorylibrarylistwidgetitem.h"

#include <librepcb/common/network/networkrequest.h>
#include <librepcb/library/library.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace manager {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

RepositoryLibraryListWidgetItem::RepositoryLibraryListWidgetItem(
    workspace::Workspace& ws, const QJsonObject& obj) noexcept
  : QWidget(nullptr),
    mWorkspace(ws),
    mJsonObject(obj),
    mUi(new Ui::RepositoryLibraryListWidgetItem) {
  mUi->setupUi(this);
  mUi->lblIcon->setText("");
  mUi->prgProgress->setVisible(false);
  connect(mUi->cbxDownload, &QCheckBox::toggled, this,
          &RepositoryLibraryListWidgetItem::checkedChanged);

  mUuid    = Uuid::tryFromString(mJsonObject.value("uuid").toString());
  mVersion = Version::tryFromString(mJsonObject.value("version").toString());
  mIsRecommended = mJsonObject.value("recommended").toBool();
  QString name =
      mJsonObject.value("name").toObject().value("default").toString();
  QString desc =
      mJsonObject.value("description").toObject().value("default").toString();
  QString author  = mJsonObject.value("author").toString();
  QUrl    iconUrl = QUrl(mJsonObject.value("icon_url").toString());
  foreach (const QJsonValue& value,
           mJsonObject.value("dependencies").toArray()) {
    tl::optional<Uuid> uuid = Uuid::tryFromString(value.toString());
    if (uuid) {
      mDependencies.insert(*uuid);
    } else {
      qWarning() << "Invalid dependency UUID:" << value.toString();
    }
  }

  mUi->lblName->setText(
      QString("%1 v%2").arg(name, mVersion ? mVersion->toStr() : QString()));
  mUi->lblDescription->setText(desc);
  mUi->lblAuthor->setText(QString("Author: %1").arg(author));

  NetworkRequest* request = new NetworkRequest(iconUrl);
  connect(request, &NetworkRequest::dataReceived, this,
          &RepositoryLibraryListWidgetItem::iconReceived, Qt::QueuedConnection);
  request->start();

  // check if this library is already installed
  updateInstalledStatus();
  connect(&mWorkspace.getLibraryDb(),
          &workspace::WorkspaceLibraryDb::scanLibraryListUpdated, this,
          &RepositoryLibraryListWidgetItem::updateInstalledStatus);
}

RepositoryLibraryListWidgetItem::~RepositoryLibraryListWidgetItem() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool RepositoryLibraryListWidgetItem::isChecked() const noexcept {
  return mUi->cbxDownload->isChecked();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void RepositoryLibraryListWidgetItem::setChecked(bool checked) noexcept {
  mUi->cbxDownload->setChecked(checked);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void RepositoryLibraryListWidgetItem::startDownloadIfSelected() noexcept {
  if (mUuid && mUi->cbxDownload->isVisible() && mUi->cbxDownload->isChecked() &&
      (!mLibraryDownload)) {
    mUi->cbxDownload->setVisible(false);
    mUi->prgProgress->setVisible(true);

    // read ZIP metadata from JSON
    QUrl       url     = QUrl(mJsonObject.value("download_url").toString());
    qint64     zipSize = mJsonObject.value("download_size").toInt(-1);
    QByteArray zipSha256 =
        mJsonObject.value("download_sha256").toString().toUtf8();

    // determine destination directory
    QString  libDirName = mUuid->toStr() % ".lplib";
    FilePath destDir =
        mWorkspace.getLibrariesPath().getPathTo("remote/" % libDirName);

    // start download
    mLibraryDownload.reset(new LibraryDownload(url, destDir));
    if (zipSize > 0) {
      mLibraryDownload->setExpectedZipFileSize(zipSize);
    }
    if (!zipSha256.isEmpty()) {
      mLibraryDownload->setExpectedChecksum(QCryptographicHash::Sha256,
                                            QByteArray::fromHex(zipSha256));
    }
    connect(mLibraryDownload.data(), &LibraryDownload::progressPercent,
            mUi->prgProgress, &QProgressBar::setValue, Qt::QueuedConnection);
    connect(mLibraryDownload.data(), &LibraryDownload::finished, this,
            &RepositoryLibraryListWidgetItem::downloadFinished,
            Qt::QueuedConnection);
    mLibraryDownload->start();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void RepositoryLibraryListWidgetItem::downloadFinished(
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

void RepositoryLibraryListWidgetItem::iconReceived(
    const QByteArray& data) noexcept {
  QPixmap pixmap;
  pixmap.loadFromData(data);
  mUi->lblIcon->setPixmap(pixmap);
}

void RepositoryLibraryListWidgetItem::updateInstalledStatus() noexcept {
  // Don't update the widgets while the download is running, it would mess up
  // the UI!
  if (mLibraryDownload) {
    return;
  }

  if (mUuid) {
    tl::optional<Version> installedVersion;
    try {
      FilePath fp =
          mWorkspace.getLibraryDb().getLatestLibrary(*mUuid);  // can throw
      if (fp.isValid()) {
        Version v = Version::fromString("0.1");  // only for initialization
        mWorkspace.getLibraryDb().getElementMetadata<Library>(fp, nullptr,
                                                              &v);  // can throw
        installedVersion = v;
      }
    } catch (const Exception& e) {
      qCritical() << "Could not determine if library is installed.";
    }
    if (installedVersion) {
      if (installedVersion < mVersion) {
        mUi->lblInstalledVersion->setText(
            QString(tr("v%1")).arg(installedVersion->toStr()));
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
      if (mIsRecommended) {
        mUi->lblInstalledVersion->setText(tr("Recommended"));
        mUi->lblInstalledVersion->setStyleSheet("QLabel {color: blue;}");
        mUi->lblInstalledVersion->setVisible(true);
      } else {
        mUi->lblInstalledVersion->setVisible(false);
      }
      mUi->cbxDownload->setText(tr("Install") % ":");
      mUi->cbxDownload->setVisible(true);
    }
  } else {
    mUi->lblInstalledVersion->setText(tr("Error: Invalid UUID"));
    mUi->lblInstalledVersion->setStyleSheet("QLabel {color: red;}");
    mUi->lblInstalledVersion->setVisible(true);
    mUi->cbxDownload->setVisible(false);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace manager
}  // namespace library
}  // namespace librepcb
