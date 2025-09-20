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
#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/ziparchive.h>
#include <librepcb/core/fileio/zipwriter.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class ZipWriterZipArchiveTest : public ::testing::Test {
protected:
  FilePath mTmpDir;
  FilePath mZipFilePath;

  ZipWriterZipArchiveTest()
    : mTmpDir(FilePath::getRandomTempPath()),
      mZipFilePath(mTmpDir.getPathTo("test file.zip")) {}
  ~ZipWriterZipArchiveTest() { QDir(mTmpDir.toStr()).removeRecursively(); }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(ZipWriterZipArchiveTest, testInMemory) {
  ZipWriter w;
  w.writeFile("test dir/file 1", "a", 0644);
  w.writeFile("test dir/file 2", "b", 0644);
  w.finish();

  ZipArchive a(w.getData());
  ASSERT_EQ(2U, a.getEntriesCount());
  EXPECT_EQ("test dir/file 1", a.getFileName(0).toStdString());
  EXPECT_EQ("test dir/file 2", a.getFileName(1).toStdString());
  EXPECT_EQ("a", a.readFile(0));
  EXPECT_EQ("b", a.readFile(1));
}

TEST_F(ZipWriterZipArchiveTest, testWriteReadEmptyArchive) {
  ZipWriter w(mZipFilePath);
  w.finish();

  EXPECT_TRUE(mZipFilePath.isExistingFile());

  ZipArchive a(mZipFilePath);
  EXPECT_EQ(0U, a.getEntriesCount());
}

TEST_F(ZipWriterZipArchiveTest, testWriteReadEmptyFile) {
  ZipWriter w(mZipFilePath);
  w.writeFile("empty.txt", QByteArray(), 0644);
  w.finish();

  EXPECT_TRUE(mZipFilePath.isExistingFile());

  ZipArchive a(mZipFilePath);
  ASSERT_EQ(1U, a.getEntriesCount());
  EXPECT_EQ("empty.txt", a.getFileName(0));
  EXPECT_EQ(QByteArray(), a.readFile(0));
}

TEST_F(ZipWriterZipArchiveTest, testWriteReadLargeFile) {
  QByteArray arr;
  arr.resize(100L * 1024L * 1024L);  // 100MB
  for (int i = 0; i < arr.size(); ++i) {
    arr[i] = (i * i) % 255;
  }

  ZipWriter w(mZipFilePath);
  w.writeFile("test dir/large file.bin", arr, 0644);
  w.finish();

  EXPECT_TRUE(mZipFilePath.isExistingFile());

  ZipArchive a(mZipFilePath);
  ASSERT_EQ(1U, a.getEntriesCount());
  EXPECT_EQ("test dir/large file.bin", a.getFileName(0).toStdString());
  const QByteArray readback = a.readFile(0);
  EXPECT_EQ(arr.size(), readback.size());
  EXPECT_EQ(arr, readback);
}

TEST_F(ZipWriterZipArchiveTest, testExtractTo) {
  ZipWriter w(mZipFilePath);
  w.writeFile("test dir/file 1", "a", 0644);
  w.writeFile("test dir/file 2", "b", 0644);
  w.finish();

  EXPECT_TRUE(mZipFilePath.isExistingFile());

  ZipArchive a(mZipFilePath);
  ASSERT_EQ(2U, a.getEntriesCount());
  const FilePath dst = mTmpDir.getPathTo("sub dir");
  a.extractTo(dst);
  EXPECT_EQ("a", FileUtils::readFile(dst.getPathTo("test dir/file 1")));
  EXPECT_EQ("b", FileUtils::readFile(dst.getPathTo("test dir/file 2")));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
