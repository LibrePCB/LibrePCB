/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "repositorylibrarylistwidgetitem.h"
#include "ui_repositorylibrarylistwidgetitem.h"
#include <librepcb/common/network/networkrequest.h>
#include "../../workspace.h"
#include "librarydownload.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

RepositoryLibraryListWidgetItem::RepositoryLibraryListWidgetItem(Workspace& ws,
                                                                 const QJsonObject& obj) noexcept :
    QWidget(nullptr), mWorkspace(ws), mJsonObject(obj),
    mUi(new Ui::RepositoryLibraryListWidgetItem)
{
    mUi->setupUi(this);
    mUi->lblIcon->setText("");
    mUi->prgProgress->setVisible(false);
    connect(mUi->cbxDownload, &QCheckBox::toggled,
            this, &RepositoryLibraryListWidgetItem::checkedChanged);

    mUuid = Uuid(mJsonObject.value("uuid").toString());
    mVersion = Version(mJsonObject.value("version").toString());
    mIsRecommended = mJsonObject.value("recommended").toBool();
    QString name = mJsonObject.value("name").toObject().value("en_US").toString();
    QString desc = mJsonObject.value("description").toObject().value("en_US").toString();
    QString author = mJsonObject.value("author").toString();
    QUrl iconUrl = QUrl(mJsonObject.value("icon_url").toString());
    foreach (const QJsonValue& value, mJsonObject.value("dependencies").toArray()) {
        Uuid uuid(value.toString());
        if (!uuid.isNull()) {
            mDependencies.insert(uuid);
        } else {
            qWarning() << "Invalid dependency UUID:" << value.toString();
        }
    }

    mUi->lblName->setText(QString("%1 v%2").arg(name, mVersion.toStr()));
    mUi->lblDescription->setText(desc);
    mUi->lblAuthor->setText(QString("Author: %1").arg(author));

    NetworkRequest* request = new NetworkRequest(iconUrl);
    connect(request, &NetworkRequest::dataReceived,
            this, &RepositoryLibraryListWidgetItem::iconReceived, Qt::QueuedConnection);
    request->start();

    // check if this library is already installed
    updateInstalledStatus();
}

RepositoryLibraryListWidgetItem::~RepositoryLibraryListWidgetItem() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool RepositoryLibraryListWidgetItem::isChecked() const noexcept
{
    return mUi->cbxDownload->isChecked();
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void RepositoryLibraryListWidgetItem::setChecked(bool checked) noexcept
{
    mUi->cbxDownload->setChecked(checked);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void RepositoryLibraryListWidgetItem::updateInstalledStatus() noexcept
{
    Version installedVersion = mWorkspace.getVersionOfLibrary(mUuid, true, true);
    if (installedVersion.isValid()) {
        mUi->lblInstalledVersion->setText(QString(tr("Installed: v%1"))
                                          .arg(installedVersion.toStr()));
        mUi->lblInstalledVersion->setVisible(true);
        if (installedVersion < mVersion) {
            mUi->lblInstalledVersion->setStyleSheet("QLabel {color: red;}");
            mUi->cbxDownload->setText(tr("Update"));
            mUi->cbxDownload->setVisible(true);
        } else {
            mUi->lblInstalledVersion->setStyleSheet("QLabel {color: green;}");
            mUi->cbxDownload->setVisible(false);
        }
    } else {
        if (mIsRecommended) {
            mUi->lblInstalledVersion->setText(tr("Recommended"));
            mUi->lblInstalledVersion->setStyleSheet("QLabel {color: blue;}");
            mUi->lblInstalledVersion->setVisible(true);
        } else {
            mUi->lblInstalledVersion->setVisible(false);
        }
        mUi->cbxDownload->setText(tr("Install"));
        mUi->cbxDownload->setVisible(true);
    }
}

void RepositoryLibraryListWidgetItem::startDownloadIfSelected() noexcept
{
    if (mUi->cbxDownload->isVisible() && mUi->cbxDownload->isChecked() && (!mLibraryDownload)) {
        mUi->cbxDownload->setVisible(false);
        mUi->prgProgress->setVisible(true);

        // read ZIP metadata from JSON
        QUrl url = QUrl(mJsonObject.value("zip_url").toString());
        qint64 zipSize = mJsonObject.value("zip_size").toInt(-1);
        QByteArray zipSha256 = mJsonObject.value("zip_sha256").toString().toUtf8();

        // determine destination directory
        QString libDirName = mUuid.toStr() % ".lplib";
        FilePath destDir = mWorkspace.getLibrariesPath().getPathTo("remote/" % libDirName);

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
        connect(mLibraryDownload.data(), &LibraryDownload::finished,
                this, &RepositoryLibraryListWidgetItem::downloadFinished, Qt::QueuedConnection);
        mLibraryDownload->start();
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void RepositoryLibraryListWidgetItem::downloadFinished(bool success, const QString& errMsg) noexcept
{
    Q_ASSERT(mLibraryDownload);

    if (success) {
        try {
            // if the library exists already in the workspace, remove it first
            QString libDirName = mLibraryDownload->getDestinationDir().getFilename();
            if (mWorkspace.getRemoteLibraries().contains(libDirName)) {
                mWorkspace.removeRemoteLibrary(libDirName, false); // can throw
            }

            // add downloaded library to workspace
            mWorkspace.addRemoteLibrary(libDirName); // can throw

            // finish
            emit libraryAdded(mLibraryDownload->getDestinationDir(), false);
        } catch (const Exception& e) {
            QMessageBox::critical(this, tr("Download failed"), e.getUserMsg());
        }
    } else if (!errMsg.isEmpty()) {
        QMessageBox::critical(this, tr("Download failed"), errMsg);
    }

    // update widgets
    mUi->cbxDownload->setChecked(!success);
    mUi->cbxDownload->setVisible(true);
    mUi->prgProgress->setVisible(false);
    updateInstalledStatus();

    // delete download helper
    mLibraryDownload.reset();
}

void RepositoryLibraryListWidgetItem::iconReceived(const QByteArray& data) noexcept
{
    QPixmap pixmap;
    pixmap.loadFromData(data);
    mUi->lblIcon->setPixmap(pixmap);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
