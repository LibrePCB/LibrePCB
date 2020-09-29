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
#include <gtest/gtest.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/fileio/transactionalfilesystem.h>
#include <librepcb/common/network/networkaccessmanager.h>
#include <librepcb/librarymanager/librarydownload.h>

#include <QSignalSpy>
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace manager {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class LibraryDownloadTest : public ::testing::Test {
public:
  static void SetUpTestCase() { sDownloadManager = new NetworkAccessManager(); }

  static void TearDownTestCase() { delete sDownloadManager; }

  static void createZip(const FilePath& dir, const FilePath& zip) {
    std::shared_ptr<TransactionalFileSystem> fs =
        TransactionalFileSystem::openRO(dir);
    fs->exportToZip(zip);
  }

protected:
  static NetworkAccessManager* sDownloadManager;
};

NetworkAccessManager* LibraryDownloadTest::sDownloadManager = nullptr;

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(LibraryDownloadTest, testDownloadInvalidLibrary) {
  // create temporary directory
  FilePath dstDir = FilePath::getRandomTempPath();
  FilePath dstLibDir = dstDir.getPathTo("my library");
  FileUtils::makePath(dstDir);

  // prepare library ZIP
  FilePath srcLibZip = dstDir.getPathTo("lib.zip");
  createZip(dstDir, srcLibZip);

  // start the file download
  LibraryDownload* dl =
      new LibraryDownload(QUrl::fromLocalFile(srcLibZip.toNative()), dstLibDir);
  QSignalSpy spyFinished(dl, SIGNAL(finished(bool, QString)));
  dl->start();

  // wait until download finished (with timeout)
  qint64 start = QDateTime::currentDateTime().toMSecsSinceEpoch();
  auto currentTime = []() {
    return QDateTime::currentDateTime().toMSecsSinceEpoch();
  };
  while ((spyFinished.isEmpty()) && (currentTime() - start < 30000)) {
    QThread::msleep(100);
    qApp->processEvents();
  }

  // check count and parameters of emited signals
  EXPECT_EQ(1, spyFinished.count());
  EXPECT_FALSE(spyFinished.first()[0].toBool());  // success
  EXPECT_FALSE(spyFinished.first()[1].toString().isEmpty())
      << spyFinished.first()[1].toString().toStdString();  // error message
  EXPECT_FALSE(dstLibDir.isExistingDir());
}

TEST_F(LibraryDownloadTest, testDownloadValidLibrary) {
  // create temporary directory
  FilePath dstDir = FilePath::getRandomTempPath();
  FilePath dstLibDir = dstDir.getPathTo("my library");
  FileUtils::makePath(dstDir);

  // prepare library ZIP
  FilePath srcLibDir(TEST_DATA_DIR "/libraries/Populated Library.lplib");
  FilePath srcLibZip = dstDir.getPathTo("lib.zip");
  createZip(srcLibDir, srcLibZip);

  // start the file download
  LibraryDownload* dl =
      new LibraryDownload(QUrl::fromLocalFile(srcLibZip.toNative()), dstLibDir);
  QSignalSpy spyFinished(dl, SIGNAL(finished(bool, QString)));
  dl->start();

  // wait until download finished (with timeout)
  qint64 start = QDateTime::currentDateTime().toMSecsSinceEpoch();
  auto currentTime = []() {
    return QDateTime::currentDateTime().toMSecsSinceEpoch();
  };
  while ((spyFinished.isEmpty()) && (currentTime() - start < 30000)) {
    QThread::msleep(100);
    qApp->processEvents();
  }

  // check count and parameters of emited signals
  EXPECT_EQ(1, spyFinished.count());
  EXPECT_TRUE(spyFinished.first()[0].toBool());  // success
  EXPECT_TRUE(spyFinished.first()[1].toString().isNull())
      << spyFinished.first()[1].toString().toStdString();  // error message
  EXPECT_TRUE(dstLibDir.isExistingDir());
  EXPECT_TRUE(dstLibDir.getPathTo(".librepcb-lib").isExistingFile());
}

TEST_F(LibraryDownloadTest, testDownloadValidNestedLibrary) {
  // create temporary directory
  FilePath dstDir = FilePath::getRandomTempPath();
  FilePath dstLibDir = dstDir.getPathTo("my library");
  FileUtils::makePath(dstDir);

  // prepare library ZIP
  FilePath srcLibDir(TEST_DATA_DIR "/libraries/Populated Library.lplib");
  FilePath tmpLibDir = dstDir.getPathTo("temp dir");
  FileUtils::copyDirRecursively(srcLibDir, tmpLibDir);
  FilePath srcLibZip = dstDir.getPathTo("lib.zip");
  createZip(dstDir, srcLibZip);

  // start the file download
  LibraryDownload* dl =
      new LibraryDownload(QUrl::fromLocalFile(srcLibZip.toNative()), dstLibDir);
  QSignalSpy spyFinished(dl, SIGNAL(finished(bool, QString)));
  dl->start();

  // wait until download finished (with timeout)
  qint64 start = QDateTime::currentDateTime().toMSecsSinceEpoch();
  auto currentTime = []() {
    return QDateTime::currentDateTime().toMSecsSinceEpoch();
  };
  while ((spyFinished.isEmpty()) && (currentTime() - start < 30000)) {
    QThread::msleep(100);
    qApp->processEvents();
  }

  // check count and parameters of emited signals
  EXPECT_EQ(1, spyFinished.count());
  EXPECT_TRUE(spyFinished.first()[0].toBool());  // success
  EXPECT_TRUE(spyFinished.first()[1].toString().isNull())
      << spyFinished.first()[1].toString().toStdString();  // error message
  EXPECT_TRUE(dstLibDir.isExistingDir());
  EXPECT_TRUE(dstLibDir.getPathTo(".librepcb-lib").isExistingFile());
}

TEST_F(LibraryDownloadTest, testDownloadValidLibraryDestinationAlreadyExists) {
  // create temporary directory
  FilePath dstDir = FilePath::getRandomTempPath();
  FilePath dstLibDir = dstDir.getPathTo("my library");
  FileUtils::makePath(dstDir);

  // prepare library ZIP
  FilePath srcLibDir(TEST_DATA_DIR "/libraries/Populated Library.lplib");
  FilePath srcLibZip = dstDir.getPathTo("lib.zip");
  createZip(srcLibDir, srcLibZip);

  // create destination directory, temporary destination directory, and ZIP
  // to check if the library download overwrites them all
  FilePath dstTmpDir = FilePath(dstLibDir.toStr() % ".tmp");
  FilePath dstZip = FilePath(dstLibDir.toStr() % ".zip");
  FileUtils::makePath(dstLibDir);
  FileUtils::makePath(dstTmpDir);
  FileUtils::writeFile(dstZip, QByteArray());

  // start the file download
  LibraryDownload* dl =
      new LibraryDownload(QUrl::fromLocalFile(srcLibZip.toNative()), dstLibDir);
  QSignalSpy spyFinished(dl, SIGNAL(finished(bool, QString)));
  dl->start();

  // wait until download finished (with timeout)
  qint64 start = QDateTime::currentDateTime().toMSecsSinceEpoch();
  auto currentTime = []() {
    return QDateTime::currentDateTime().toMSecsSinceEpoch();
  };
  while ((spyFinished.isEmpty()) && (currentTime() - start < 30000)) {
    QThread::msleep(100);
    qApp->processEvents();
  }

  // check count and parameters of emited signals
  EXPECT_EQ(1, spyFinished.count());
  EXPECT_TRUE(spyFinished.first()[0].toBool());  // success
  EXPECT_TRUE(spyFinished.first()[1].toString().isNull())
      << spyFinished.first()[1].toString().toStdString();  // error message
  EXPECT_TRUE(dstLibDir.isExistingDir());
  EXPECT_TRUE(dstLibDir.getPathTo(".librepcb-lib").isExistingFile());
  EXPECT_FALSE(dstTmpDir.isExistingDir());
  EXPECT_FALSE(dstZip.isExistingFile());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace manager
}  // namespace library
}  // namespace librepcb
