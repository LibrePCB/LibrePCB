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
#include "networkrequestbasesignalreceiver.h"

#include <gtest/gtest.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/network/filedownload.h>
#include <librepcb/core/network/networkaccessmanager.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Data Type
 ******************************************************************************/

typedef struct {
  const char* url;
  const char* destFilename;
  const char* extractDirname;
  const char* sha256;
  const char* errorMsg;
  bool testExistingDestination;
} FileDownloadTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class FileDownloadTest : public ::testing::TestWithParam<FileDownloadTestData> {
protected:
  FilePath mTmpDir;
  NetworkRequestBaseSignalReceiver mSignalReceiver;
  static NetworkAccessManager* sDownloadManager;

  FileDownloadTest() : mTmpDir(FilePath::getRandomTempPath()) {}

  ~FileDownloadTest() { QDir(mTmpDir.toStr()).removeRecursively(); }

  FilePath getDestination(const FileDownloadTestData& data) {
    return mTmpDir.getPathTo(data.destFilename);
  }

  FilePath getExtractToDir(const FileDownloadTestData& data) {
    if (data.extractDirname) {
      return mTmpDir.getPathTo(data.extractDirname);
    } else {
      return FilePath();
    }
  }

  static void SetUpTestCase() { sDownloadManager = new NetworkAccessManager(); }

  static void TearDownTestCase() { delete sDownloadManager; }
};

NetworkAccessManager* FileDownloadTest::sDownloadManager = nullptr;

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(FileDownloadTest, testConstructorAndSettersAndDestructor) {
  const FileDownloadTestData& data = GetParam();

  FileDownload dl(QUrl::fromLocalFile(data.url), getDestination(data),
                  std::make_shared<QSemaphore>(1));
  dl.setExpectedReplyContentSize(100);
  dl.setExpectedChecksum(QCryptographicHash::Sha1, QByteArray("42"));
  dl.setZipExtractionDirectory(getExtractToDir(data));
}

TEST_P(FileDownloadTest, testDownload) {
  const FileDownloadTestData& data = GetParam();

  // create destination files
  if (data.testExistingDestination) {
    FileUtils::writeFile(getDestination(data), "Foo");
    FileUtils::writeFile(getExtractToDir(data).getPathTo("foo"), "Foo");
  }

  // start the file download
  FileDownload* dl =
      new FileDownload(QUrl::fromLocalFile(data.url), getDestination(data),
                       std::make_shared<QSemaphore>(1));
  dl->setZipExtractionDirectory(getExtractToDir(data));
  if (data.sha256) {
    dl->setExpectedChecksum(QCryptographicHash::Sha256,
                            QByteArray::fromHex(data.sha256));
  }
  QObject::connect(dl, &FileDownload::progressState, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::progressState);
  QObject::connect(dl, &FileDownload::progressPercent, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::progressPercent);
  QObject::connect(dl, &FileDownload::progress, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::progress);
  QObject::connect(dl, &FileDownload::aborted, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::aborted);
  QObject::connect(dl, &FileDownload::succeeded, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::succeeded);
  QObject::connect(dl, &FileDownload::errored, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::errored);
  QObject::connect(dl, &FileDownload::finished, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::finished);
  QObject::connect(dl, &FileDownload::fileDownloaded, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::fileDownloaded);
  QObject::connect(dl, &FileDownload::zipFileExtracted, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::zipFileExtracted);
  QObject::connect(dl, &FileDownload::destroyed, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::destroyed);
  dl->start();

  // wait until download finished (with timeout)
  qint64 start = QDateTime::currentMSecsSinceEpoch();
  auto currentTime = []() { return QDateTime::currentMSecsSinceEpoch(); };
  while ((!mSignalReceiver.mDestroyed) && (currentTime() - start < 30000)) {
    QThread::msleep(100);
    qApp->processEvents();
  }

  // check count and parameters of emitted signals
  EXPECT_TRUE(mSignalReceiver.mDestroyed) << "Download timed out!";
  EXPECT_GT(mSignalReceiver.mProgressStateCallCount, 0);
  EXPECT_EQ(mSignalReceiver.mAdvancedProgressCallCount,
            mSignalReceiver.mSimpleProgressCallCount);
  EXPECT_EQ(0, mSignalReceiver.mAbortedCallCount);
  EXPECT_EQ(1, mSignalReceiver.mFinishedCallCount);
  EXPECT_EQ(0, mSignalReceiver.mDataReceivedCallCount);
  EXPECT_TRUE(mSignalReceiver.mReceivedData.isNull())
      << qPrintable(mSignalReceiver.mReceivedData);
  EXPECT_TRUE(mSignalReceiver.mReceivedContentType.isEmpty());
  EXPECT_EQ(std::string(data.errorMsg ? data.errorMsg : ""),
            mSignalReceiver.mErrorMessage.toStdString());
  if (!data.errorMsg) {
    EXPECT_GE(mSignalReceiver.mSimpleProgressCallCount, 1);
    EXPECT_EQ(1, mSignalReceiver.mSucceededCallCount);
    EXPECT_EQ(0, mSignalReceiver.mErroredCallCount);
    EXPECT_EQ(1, mSignalReceiver.mFileDownloadedCallCount);
    EXPECT_TRUE(mSignalReceiver.mFinishedSuccess);
    EXPECT_EQ(getDestination(data), mSignalReceiver.mDownloadedToFilePath);
    EXPECT_EQ(getExtractToDir(data), mSignalReceiver.mExtractedToFilePath);
    EXPECT_EQ(data.extractDirname == nullptr,
              getDestination(data).isExistingFile());
  } else {
    EXPECT_GE(mSignalReceiver.mSimpleProgressCallCount, 0);
    EXPECT_EQ(0, mSignalReceiver.mSucceededCallCount);
    EXPECT_EQ(1, mSignalReceiver.mErroredCallCount);
    EXPECT_EQ(0, mSignalReceiver.mFileDownloadedCallCount);
    EXPECT_FALSE(mSignalReceiver.mFinishedSuccess);
    EXPECT_FALSE(getDestination(data).isExistingFile());
  }
  if ((!data.errorMsg) && data.extractDirname) {
    EXPECT_EQ(1, mSignalReceiver.mZipFileExtractedCallCount);
    EXPECT_TRUE(getExtractToDir(data).isExistingDir());
    EXPECT_FALSE(getExtractToDir(data).isEmptyDir());
  } else {
    EXPECT_EQ(0, mSignalReceiver.mZipFileExtractedCallCount);
    EXPECT_FALSE(getExtractToDir(data).isExistingDir());
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(FileDownloadTest, FileDownloadTest, ::testing::Values(
    FileDownloadTestData({TEST_DATA_DIR "/unittests/librepcbcommon/FileDownloadTest/first_pcb.zip",
                          "first_pcb_downloaded.zip",
                          "first_pcb_extracted",
                          "f6f18782790d2a185698f7028a83397d56ef6145679f646c8de5ddfc298d8f89",
                          nullptr, // no error
                          false}),
    FileDownloadTestData({TEST_DATA_DIR "/unittests/librepcbcommon/FileDownloadTest/first_pcb.zip",
                          "first_pcb_downloaded.zip",
                          "first_pcb_extracted",
                          "f6f18782790d2a185698f7028a83397d56ef6145679f646c8de5ddfc298d8f89",
                          nullptr, // no error
                          true}),
    FileDownloadTestData({TEST_DATA_DIR "/unittests/librepcbcommon/FileDownloadTest/first_pcb.zip",
                          "first_pcb_downloaded.zip",
                          nullptr, // no extract dir
                          "f6f18782790d2a185698f7028a83397d56ef6145679f646c8de5ddfc298d8f88", // wrong
                          "Checksum verification of downloaded file failed!",
                          false}),
    FileDownloadTestData({TEST_DATA_DIR "/unittests/librepcbcommon/FileDownloadTest/libraries",
                          "libraries.json",
                          nullptr, // no extract dir
                          nullptr, // no sha256
                          nullptr, // no error
                          false}),
    FileDownloadTestData({"/some-invalid-url",
                          "some-invalid-url",
                          "some-invalid-url_extracted",
                          nullptr, // no sha256
                          "Error opening /some-invalid-url: No such file or directory (203)",
                          false})
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
