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
#include <memory>
#include <QtCore>
#include <gtest/gtest.h>
#include <librepcb/common/network/networkaccessmanager.h>
#include <librepcb/common/network/filedownload.h>
#include <librepcb/common/fileio/fileutils.h>
#include "networkrequestbasesignalreceiver.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace tests {

/*****************************************************************************************
 *  Test Data Type
 ****************************************************************************************/

typedef struct {
    QUrl url;
    QString destFilename;
    QString extractDirname;
    QByteArray sha256;
    bool success;
} FileDownloadTestData;

/*****************************************************************************************
 *  Test Class
 ****************************************************************************************/

class FileDownloadTest : public ::testing::TestWithParam<FileDownloadTestData>
{
    public:

        static void SetUpTestCase() {
            sDownloadManager = std::unique_ptr<NetworkAccessManager>(new NetworkAccessManager());
        }

        static void TearDownTestCase() {
        }

        static FilePath getDestination(const FileDownloadTestData& data) {
            return FilePath::getApplicationTempPath().getPathTo(data.destFilename);
        }

        static FilePath getExtractToDir(const FileDownloadTestData& data) {
            if (!data.extractDirname.isEmpty()) {
                return FilePath::getApplicationTempPath().getPathTo(data.extractDirname);
            } else {
                return FilePath();
            }
        }

    protected:

        NetworkRequestBaseSignalReceiver mSignalReceiver;
        static std::unique_ptr<NetworkAccessManager> sDownloadManager;
};

std::unique_ptr<NetworkAccessManager> FileDownloadTest::sDownloadManager = nullptr;

/*****************************************************************************************
 *  Test Methods
 ****************************************************************************************/

TEST_P(FileDownloadTest, testConstructorAndSettersAndDestructor)
{
    const FileDownloadTestData& data = GetParam();

    FileDownload dl(data.url, getDestination(data));
    dl.setExpectedReplyContentSize(100);
    dl.setExpectedChecksum(QCryptographicHash::Sha1, QByteArray("42"));
    dl.setZipExtractionDirectory(getExtractToDir(data));
}

TEST_P(FileDownloadTest, testDownload)
{
    const FileDownloadTestData& data = GetParam();

    // remove target file/directory
    if (getDestination(data).isExistingFile()) {
        FileUtils::removeFile(getDestination(data));
    }
    if (getExtractToDir(data).isExistingDir()) {
        FileUtils::removeDirRecursively(getExtractToDir(data));
    }

    // start the file download
    FileDownload* dl = new FileDownload(data.url, getDestination(data));
    dl->setZipExtractionDirectory(getExtractToDir(data));
    dl->setExpectedChecksum(QCryptographicHash::Sha256, data.sha256);
    QObject::connect(dl, &FileDownload::progressState,
            &mSignalReceiver, &NetworkRequestBaseSignalReceiver::progressState);
    QObject::connect(dl, &FileDownload::progressPercent,
            &mSignalReceiver, &NetworkRequestBaseSignalReceiver::progressPercent);
    QObject::connect(dl, &FileDownload::progress,
            &mSignalReceiver, &NetworkRequestBaseSignalReceiver::progress);
    QObject::connect(dl, &FileDownload::aborted,
            &mSignalReceiver, &NetworkRequestBaseSignalReceiver::aborted);
    QObject::connect(dl, &FileDownload::succeeded,
            &mSignalReceiver, &NetworkRequestBaseSignalReceiver::succeeded);
    QObject::connect(dl, &FileDownload::errored,
            &mSignalReceiver, &NetworkRequestBaseSignalReceiver::errored);
    QObject::connect(dl, &FileDownload::finished,
            &mSignalReceiver, &NetworkRequestBaseSignalReceiver::finished);
    QObject::connect(dl, &FileDownload::fileDownloaded,
            &mSignalReceiver, &NetworkRequestBaseSignalReceiver::fileDownloaded);
    QObject::connect(dl, &FileDownload::zipFileExtracted,
            &mSignalReceiver, &NetworkRequestBaseSignalReceiver::zipFileExtracted);
    QObject::connect(dl, &FileDownload::destroyed,
            &mSignalReceiver, &NetworkRequestBaseSignalReceiver::destroyed);
    dl->start();

    // wait until download finished (with timeout)
    qint64 start = QDateTime::currentDateTime().toMSecsSinceEpoch();
    auto currentTime = [](){return QDateTime::currentDateTime().toMSecsSinceEpoch();};
    while ((!mSignalReceiver.mDestroyed) && (currentTime() - start < 30000)) {
        QThread::msleep(100);
        qApp->processEvents();
    }

    // check count and parameters of emited signals
    EXPECT_TRUE(mSignalReceiver.mDestroyed) << "Download timed out!";
    EXPECT_GT(mSignalReceiver.mProgressStateCallCount, 0);
    EXPECT_EQ(mSignalReceiver.mAdvancedProgressCallCount,
              mSignalReceiver.mSimpleProgressCallCount);
    EXPECT_EQ(0, mSignalReceiver.mAbortedCallCount);
    EXPECT_EQ(1, mSignalReceiver.mFinishedCallCount);
    EXPECT_EQ(0, mSignalReceiver.mDataReceivedCallCount);
    EXPECT_TRUE(mSignalReceiver.mReceivedData.isNull())
            << qPrintable(mSignalReceiver.mReceivedData);
    if (data.success) {
        EXPECT_GE(mSignalReceiver.mSimpleProgressCallCount, 1);
        EXPECT_EQ(1, mSignalReceiver.mSucceededCallCount);
        EXPECT_EQ(0, mSignalReceiver.mErroredCallCount);
        EXPECT_EQ(1, mSignalReceiver.mFileDownloadedCallCount);
        EXPECT_TRUE(mSignalReceiver.mErrorMessage.isNull())
                << qPrintable(mSignalReceiver.mErrorMessage);
        EXPECT_TRUE(mSignalReceiver.mFinishedSuccess);
        EXPECT_EQ(getDestination(data),     mSignalReceiver.mDownloadedToFilePath);
        EXPECT_EQ(getExtractToDir(data),    mSignalReceiver.mExtractedToFilePath);
        EXPECT_EQ(data.extractDirname.isNull(), getDestination(data).isExistingFile());
    } else {
        EXPECT_GE(mSignalReceiver.mSimpleProgressCallCount, 0);
        EXPECT_EQ(0, mSignalReceiver.mSucceededCallCount);
        EXPECT_EQ(1, mSignalReceiver.mErroredCallCount);
        EXPECT_EQ(0, mSignalReceiver.mFileDownloadedCallCount);
        EXPECT_FALSE(mSignalReceiver.mErrorMessage.isEmpty())
                << qPrintable(mSignalReceiver.mErrorMessage);
        EXPECT_FALSE(mSignalReceiver.mFinishedSuccess);
        EXPECT_FALSE(getDestination(data).isExistingFile());
    }
    if (data.success && (!data.extractDirname.isNull())) {
        EXPECT_EQ(1, mSignalReceiver.mZipFileExtractedCallCount);
        EXPECT_TRUE(getExtractToDir(data).isExistingDir());
        EXPECT_FALSE(getExtractToDir(data).isEmptyDir());
    } else {
        EXPECT_EQ(0, mSignalReceiver.mZipFileExtractedCallCount);
        EXPECT_FALSE(getExtractToDir(data).isExistingDir());
    }
}

/*****************************************************************************************
 *  Test Data
 ****************************************************************************************/

INSTANTIATE_TEST_CASE_P(FileDownloadTest, FileDownloadTest, ::testing::Values(
    FileDownloadTestData({QUrl("https://github.com/LibrePCB/LibrePCB/archive/first_pcb.zip"),
                          QString("first_pcb_downloaded.zip"),
                          QString("first_pcb_extracted"),
                          QByteArray::fromHex("f6f18782790d2a185698f7028a83397d56ef6145679f646c8de5ddfc298d8f89"),
                          true}),
    FileDownloadTestData({QUrl("https://github.com/LibrePCB/LibrePCB/archive/first_pcb.zip"),
                          QString("first_pcb_downloaded.zip"),
                          QString(),
                          QByteArray::fromHex("f6f18782790d2a185698f7028a83397d56ef6145679f646c8de5ddfc298d8f88"), // wrong
                          false}),
    FileDownloadTestData({QUrl("https://api.librepcb.org/api/v1/libraries"),
                          QString("libraries.json"),
                          QString(),
                          QByteArray(),
                          true}),
    FileDownloadTestData({QUrl("https://github.com/LibrePCB/some-invalid-url"),
                          QString("some-invalid-url"),
                          QString("some-invalid-url_extracted"),
                          QByteArray(),
                          false})
));

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace librepcb
