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
#include <quazip/JlCompress.h>
#include "filedownload.h"
#include "scopeguard.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FileDownload::FileDownload(const QUrl& url, const FilePath& dest) noexcept :
    NetworkRequestBase(url), mDestination(dest), mHashAlgorithm(QCryptographicHash::Md5),
    mExpectedChecksum(), mExtractZipToDir()
{
}

FileDownload::~FileDownload() noexcept
{
}

/*****************************************************************************************
 *  Public Methods
 ****************************************************************************************/

void FileDownload::setExpectedChecksum(QCryptographicHash::Algorithm algorithm,
                                       const QByteArray& checksum) noexcept
{
    Q_ASSERT(!mStarted);
    mHashAlgorithm = algorithm;
    mExpectedChecksum = checksum;
}

void FileDownload::setZipExtractionDirectory(const FilePath& dir) noexcept
{
    Q_ASSERT(!mStarted);
    mExtractZipToDir = dir;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void FileDownload::prepareRequest()
{
    // check destination filepath
    if (mDestination.isExistingFile() || mDestination.isExistingDir()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString("The destination file exists already: %1")
            .arg(mDestination.toNative()));
    }

    // create destination directory
    if (!mDestination.getParentDir().isEmptyDir()) {
        if (!QDir().mkpath(mDestination.getParentDir().toStr())) {
            throw RuntimeError(__FILE__, __LINE__,
                QString("Could not create directory \"%1\".")
                .arg(mDestination.getParentDir().toNative()));
        }
    }

    // open temporary destination file
    mFile.reset(new QSaveFile(mDestination.toStr(), this));
    if (!mFile->open(QIODevice::WriteOnly)) {
        throw RuntimeError(__FILE__, __LINE__,
            QString("Could not open file \"%1\": %2")
            .arg(mDestination.toNative(), mFile->errorString()));
    }
}

void FileDownload::finalizeRequest()
{
    // check destination filepath again
    if (mDestination.isExistingFile() || mDestination.isExistingDir()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString("The destination file exists already: %1")
            .arg(mDestination.toNative()));
    }

    // save to destination file
    if (!mFile->commit()) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Error while writing file \"%1\": %2"))
            .arg(mDestination.toNative(), mFile->errorString()));
    }

    // if an error occurs below this line, remove the downloaded file
    auto sg = scopeGuard([this](){QFile::remove(mDestination.toStr());});

    // verify checksum of downloaded file
    if (!mExpectedChecksum.isEmpty()) {
        emit progressState(tr("Verify checksum..."));
        QFile file(mDestination.toStr());
        if (!file.open(QFile::ReadOnly)) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("Error while readback file \"%1\": %2"))
                .arg(mDestination.toNative(), file.errorString()));
        }
        QCryptographicHash hash(mHashAlgorithm);
        hash.addData(&file);
        QString result = hash.result().toHex();
        QString expected = mExpectedChecksum.toHex();
        if (result != expected) {
            qDebug() << "expected" << expected << "but got" << result;
            throw RuntimeError(__FILE__, __LINE__,
                tr("Checksum verification of downloaded file failed!"));
        } else {
            qDebug() << "Checksum verification of downloaded file was successful.";
        }
    }

    // extract zip file if neccessary
    if (mExtractZipToDir.isValid()) {
        emit progressState(tr("Extract files..."));
        QStringList files = JlCompress::extractDir(mDestination.toStr(),
                                                   mExtractZipToDir.toStr());
        if (files.isEmpty()) {
            throw RuntimeError(__FILE__, __LINE__,
                QString(tr("Error while extracting the ZIP file \"%1\"."))
                .arg(mDestination.toNative()));
        }
    } else {
        // do NOT remove the downloaded file
        sg.dismiss();
    }
}

void FileDownload::emitSuccessfullyFinishedSignals() noexcept
{
    emit fileDownloaded(mDestination);
    if (mExtractZipToDir.isValid()) {
        emit zipFileExtracted(mExtractZipToDir);
    }
}

void FileDownload::fetchNewData() noexcept
{
    mFile->write(mReply->readAll());
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
