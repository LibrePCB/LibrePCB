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
#include <librepcb/common/fileio/transactionaldirectory.h>
#include <librepcb/common/fileio/transactionalfilesystem.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class TransactionalDirectoryTest : public ::testing::Test {
protected:
  std::shared_ptr<TransactionalFileSystem> mFileSystem;
  std::shared_ptr<TransactionalFileSystem> mEmptyFileSystem;

  TransactionalDirectoryTest() {
    // Open in read-only mode to avoid creating a ".lock" file which would
    // influence the tests.
    mFileSystem =
        TransactionalFileSystem::openRO(FilePath::getRandomTempPath());
    mFileSystem->write("a.txt", "a");
    mFileSystem->write("a/b.txt", "b");
    mFileSystem->write("a/b/c.txt", "c");
    mFileSystem->write("a/b/c/d.txt", "d");

    mEmptyFileSystem =
        TransactionalFileSystem::openRO(FilePath::getRandomTempPath());
  }

  virtual ~TransactionalDirectoryTest() {}
};

/*******************************************************************************
 *  Test Constructors
 ******************************************************************************/

TEST_F(TransactionalDirectoryTest, testDefaultConstructorCreatesTempFs) {
  TransactionalDirectory dir;
  ASSERT_TRUE(dir.getFileSystem());
  EXPECT_TRUE(dir.getFileSystem()->getAbsPath().isLocatedInDir(
      FilePath::getApplicationTempPath()));
}

TEST_F(TransactionalDirectoryTest, testDefaultConstructorCreatesEmptyFs) {
  TransactionalDirectory dir;
  EXPECT_EQ(QStringList{}, dir.getDirs());
  EXPECT_EQ(QStringList{}, dir.getFiles());
}

TEST_F(TransactionalDirectoryTest, testCopyConstructorWithDefaultPath) {
  TransactionalDirectory dir(mFileSystem, "foo");
  TransactionalDirectory copy(dir);
  EXPECT_EQ(mFileSystem, copy.getFileSystem());
  EXPECT_EQ("foo", copy.getPath());
}

TEST_F(TransactionalDirectoryTest, testCopyConstructorWithPath) {
  TransactionalDirectory dir(mFileSystem, "foo");
  TransactionalDirectory copy(dir, "bar");
  EXPECT_EQ(mFileSystem, copy.getFileSystem());
  EXPECT_EQ("foo/bar", copy.getPath());
}

TEST_F(TransactionalDirectoryTest, testConstructorWithDefaultPath) {
  TransactionalDirectory dir(mFileSystem);
  EXPECT_EQ(mFileSystem, dir.getFileSystem());
  EXPECT_EQ("", dir.getPath());
}

TEST_F(TransactionalDirectoryTest, testConstructorWithPath) {
  TransactionalDirectory dir(mFileSystem, "foo");
  EXPECT_EQ(mFileSystem, dir.getFileSystem());
  EXPECT_EQ("foo", dir.getPath());
}

TEST_F(TransactionalDirectoryTest, testConstructorRemovesTrailingSlashes) {
  TransactionalDirectory dir(mFileSystem, "foo///");
  EXPECT_EQ(mFileSystem, dir.getFileSystem());
  EXPECT_EQ("foo", dir.getPath());
}

/*******************************************************************************
 *  Test getAbsPath()
 ******************************************************************************/

TEST_F(TransactionalDirectoryTest, testGetAbsPathInRoot) {
  TransactionalDirectory dir(mFileSystem, "");
  EXPECT_EQ(mFileSystem->getAbsPath(), dir.getAbsPath());
}

TEST_F(TransactionalDirectoryTest, testGetAbsPathInRootPath) {
  TransactionalDirectory dir(mFileSystem, "");
  EXPECT_EQ(mFileSystem->getAbsPath("hello"), dir.getAbsPath("hello"));
}

TEST_F(TransactionalDirectoryTest, testGetAbsPathInSubdir) {
  TransactionalDirectory dir(mFileSystem, "foo bar");
  EXPECT_EQ(mFileSystem->getAbsPath("foo bar"), dir.getAbsPath());
}

TEST_F(TransactionalDirectoryTest, testGetAbsPathInSubdirPath) {
  TransactionalDirectory dir(mFileSystem, "foo bar");
  EXPECT_EQ(mFileSystem->getAbsPath("foo bar/hello"), dir.getAbsPath("hello"));
}

/*******************************************************************************
 *  Test getDirs()
 ******************************************************************************/

TEST_F(TransactionalDirectoryTest, testGetDirsInRoot) {
  TransactionalDirectory dir(mFileSystem, "");
  EXPECT_EQ(QStringList{"a"}, dir.getDirs());
}

TEST_F(TransactionalDirectoryTest, testGetDirsInRootPath) {
  TransactionalDirectory dir(mFileSystem, "");
  EXPECT_EQ(QStringList{"b"}, dir.getDirs("a"));
}

TEST_F(TransactionalDirectoryTest, testGetDirsInSubdir) {
  TransactionalDirectory dir(mFileSystem, "a");
  EXPECT_EQ(QStringList{"b"}, dir.getDirs());
}

TEST_F(TransactionalDirectoryTest, testGetDirsInSubdirPath) {
  TransactionalDirectory dir(mFileSystem, "a");
  EXPECT_EQ(QStringList{"c"}, dir.getDirs("b"));
}

/*******************************************************************************
 *  Test getFiles()
 ******************************************************************************/

TEST_F(TransactionalDirectoryTest, testGetFilesInRoot) {
  TransactionalDirectory dir(mFileSystem, "");
  EXPECT_EQ(QStringList{"a.txt"}, dir.getFiles());
}

TEST_F(TransactionalDirectoryTest, testGetFilesInRootPath) {
  TransactionalDirectory dir(mFileSystem, "");
  EXPECT_EQ(QStringList{"b.txt"}, dir.getFiles("a"));
}

TEST_F(TransactionalDirectoryTest, testGetFilesInSubdir) {
  TransactionalDirectory dir(mFileSystem, "a");
  EXPECT_EQ(QStringList{"b.txt"}, dir.getFiles());
}

TEST_F(TransactionalDirectoryTest, testGetFilesInSubdirPath) {
  TransactionalDirectory dir(mFileSystem, "a");
  EXPECT_EQ(QStringList{"c.txt"}, dir.getFiles("b"));
}

/*******************************************************************************
 *  Test fileExists()
 ******************************************************************************/

TEST_F(TransactionalDirectoryTest, testFileExistsInRoot) {
  TransactionalDirectory dir(mFileSystem, "");
  EXPECT_TRUE(dir.fileExists("a.txt"));
  EXPECT_FALSE(dir.fileExists("b.txt"));
}

TEST_F(TransactionalDirectoryTest, testFileExistsInRootPath) {
  TransactionalDirectory dir(mFileSystem, "");
  EXPECT_TRUE(dir.fileExists("a/b.txt"));
  EXPECT_FALSE(dir.fileExists("a/c.txt"));
}

TEST_F(TransactionalDirectoryTest, testFileExistsInSubdir) {
  TransactionalDirectory dir(mFileSystem, "a");
  EXPECT_TRUE(dir.fileExists("b.txt"));
  EXPECT_FALSE(dir.fileExists("c.txt"));
}

TEST_F(TransactionalDirectoryTest, testFileExistsInSubdirPath) {
  TransactionalDirectory dir(mFileSystem, "a");
  EXPECT_TRUE(dir.fileExists("b/c.txt"));
  EXPECT_FALSE(dir.fileExists("b/d.txt"));
}

/*******************************************************************************
 *  Test read()
 ******************************************************************************/

TEST_F(TransactionalDirectoryTest, testReadInRoot) {
  TransactionalDirectory dir(mFileSystem, "");
  EXPECT_EQ("a", dir.read("a.txt"));
  EXPECT_THROW(dir.read("b.txt"), Exception);
}

TEST_F(TransactionalDirectoryTest, testReadInRootPath) {
  TransactionalDirectory dir(mFileSystem, "");
  EXPECT_EQ("b", dir.read("a/b.txt"));
  EXPECT_THROW(dir.read("a/c.txt"), Exception);
}

TEST_F(TransactionalDirectoryTest, testReadInSubdir) {
  TransactionalDirectory dir(mFileSystem, "a");
  EXPECT_EQ("b", dir.read("b.txt"));
  EXPECT_THROW(dir.read("c.txt"), Exception);
}

TEST_F(TransactionalDirectoryTest, testReadInSubdirPath) {
  TransactionalDirectory dir(mFileSystem, "a");
  EXPECT_EQ("c", dir.read("b/c.txt"));
  EXPECT_THROW(dir.read("b/d.txt"), Exception);
}

/*******************************************************************************
 *  Test write()
 ******************************************************************************/

TEST_F(TransactionalDirectoryTest, testWriteInRoot) {
  TransactionalDirectory dir(mFileSystem, "");
  dir.write("a.txt", "foo1");
  EXPECT_EQ("foo1", mFileSystem->read("a.txt"));
}

TEST_F(TransactionalDirectoryTest, testWriteInRootPath) {
  TransactionalDirectory dir(mFileSystem, "");
  dir.write("a/b.txt", "foo2");
  EXPECT_EQ("foo2", mFileSystem->read("a/b.txt"));
}

TEST_F(TransactionalDirectoryTest, testWriteInSubdir) {
  TransactionalDirectory dir(mFileSystem, "a");
  dir.write("b.txt", "foo3");
  EXPECT_EQ("foo3", mFileSystem->read("a/b.txt"));
}

TEST_F(TransactionalDirectoryTest, testWriteInSubdirPath) {
  TransactionalDirectory dir(mFileSystem, "a");
  dir.write("b/c.txt", "foo4");
  EXPECT_EQ("foo4", mFileSystem->read("a/b/c.txt"));
}

/*******************************************************************************
 *  Test removeFile()
 ******************************************************************************/

TEST_F(TransactionalDirectoryTest, testRemoveFileInRoot) {
  TransactionalDirectory dir(mFileSystem, "");
  ASSERT_TRUE(dir.fileExists("a.txt"));
  ASSERT_TRUE(mFileSystem->fileExists("a.txt"));
  dir.removeFile("a.txt");
  EXPECT_FALSE(dir.fileExists("a.txt"));
  EXPECT_FALSE(mFileSystem->fileExists("a.txt"));
}

TEST_F(TransactionalDirectoryTest, testRemoveFileInRootPath) {
  TransactionalDirectory dir(mFileSystem, "");
  ASSERT_TRUE(dir.fileExists("a/b.txt"));
  ASSERT_TRUE(mFileSystem->fileExists("a/b.txt"));
  dir.removeFile("a/b.txt");
  EXPECT_FALSE(dir.fileExists("a/b.txt"));
  EXPECT_FALSE(mFileSystem->fileExists("a/b.txt"));
}

TEST_F(TransactionalDirectoryTest, testRemoveFileInSubdir) {
  TransactionalDirectory dir(mFileSystem, "a");
  ASSERT_TRUE(dir.fileExists("b.txt"));
  ASSERT_TRUE(mFileSystem->fileExists("a/b.txt"));
  dir.removeFile("b.txt");
  EXPECT_FALSE(dir.fileExists("b.txt"));
  EXPECT_FALSE(mFileSystem->fileExists("a/b.txt"));
}

TEST_F(TransactionalDirectoryTest, testRemoveFileInSubdirPath) {
  TransactionalDirectory dir(mFileSystem, "a");
  ASSERT_TRUE(dir.fileExists("b/c.txt"));
  ASSERT_TRUE(mFileSystem->fileExists("a/b/c.txt"));
  dir.removeFile("b/c.txt");
  EXPECT_FALSE(dir.fileExists("b/c.txt"));
  EXPECT_FALSE(mFileSystem->fileExists("a/b/c.txt"));
}

/*******************************************************************************
 *  Test removeDirRecursively()
 ******************************************************************************/

TEST_F(TransactionalDirectoryTest, testRemoveDirRecursivelyInRoot) {
  TransactionalDirectory dir(mFileSystem, "");
  ASSERT_EQ(QStringList{"a"}, dir.getDirs());
  ASSERT_EQ(QStringList{"a"}, mFileSystem->getDirs());
  dir.removeDirRecursively();
  EXPECT_EQ(QStringList{}, dir.getDirs());
  EXPECT_EQ(QStringList{}, dir.getFiles());
  EXPECT_EQ(QStringList{}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{}, mFileSystem->getFiles());
}

TEST_F(TransactionalDirectoryTest, testRemoveDirRecursivelyInRootPath) {
  TransactionalDirectory dir(mFileSystem, "");
  ASSERT_EQ(QStringList{"a"}, dir.getDirs());
  ASSERT_EQ(QStringList{"a"}, mFileSystem->getDirs());
  dir.removeDirRecursively("a");
  EXPECT_EQ(QStringList{}, dir.getDirs());
  EXPECT_EQ(QStringList{}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a.txt"}, dir.getFiles());
  EXPECT_EQ(QStringList{"a.txt"}, mFileSystem->getFiles());
}

TEST_F(TransactionalDirectoryTest, testRemoveDirRecursivelyInSubdir) {
  TransactionalDirectory dir(mFileSystem, "a");
  ASSERT_EQ(QStringList{"b"}, dir.getDirs());
  ASSERT_EQ(QStringList{"a"}, mFileSystem->getDirs());
  dir.removeDirRecursively();
  EXPECT_EQ(QStringList{}, dir.getDirs());
  EXPECT_EQ(QStringList{}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{}, dir.getFiles());
  EXPECT_EQ(QStringList{"a.txt"}, mFileSystem->getFiles());
}

TEST_F(TransactionalDirectoryTest, testRemoveDirRecursivelyInSubdirPath) {
  TransactionalDirectory dir(mFileSystem, "a");
  ASSERT_EQ(QStringList{"b"}, dir.getDirs());
  ASSERT_EQ(QStringList{"a"}, mFileSystem->getDirs());
  ASSERT_EQ(QStringList{"b"}, mFileSystem->getDirs("a"));
  dir.removeDirRecursively("b");
  EXPECT_EQ(QStringList{}, dir.getDirs());
  EXPECT_EQ(QStringList{"a"}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{}, mFileSystem->getDirs("a"));
  EXPECT_EQ(QStringList{"b.txt"}, dir.getFiles());
  EXPECT_EQ(QStringList{"b.txt"}, mFileSystem->getFiles("a"));
  EXPECT_EQ(QStringList{}, dir.getFiles("b"));
  EXPECT_EQ(QStringList{}, mFileSystem->getFiles("a/b"));
  EXPECT_EQ(QStringList{"a.txt"}, mFileSystem->getFiles());
}

/*******************************************************************************
 *  Test copyTo()
 ******************************************************************************/

TEST_F(TransactionalDirectoryTest, testCopyToFromRootToRoot) {
  TransactionalDirectory src(mFileSystem, "");
  TransactionalDirectory dst(mEmptyFileSystem, "");
  src.copyTo(dst);
  EXPECT_EQ(mFileSystem, src.getFileSystem());
  EXPECT_EQ("", src.getPath());
  EXPECT_EQ(QStringList{"a"}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a.txt"}, mFileSystem->getFiles());
  EXPECT_EQ(QStringList{"b"}, mFileSystem->getDirs("a"));
  EXPECT_EQ(QStringList{"b.txt"}, mFileSystem->getFiles("a"));
  EXPECT_EQ(QStringList{"a"}, mEmptyFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a.txt"}, mEmptyFileSystem->getFiles());
  EXPECT_EQ(QStringList{"b.txt"}, mEmptyFileSystem->getFiles("a"));
}

TEST_F(TransactionalDirectoryTest, testCopyToFromRootToSubdir) {
  TransactionalDirectory src(mFileSystem, "");
  TransactionalDirectory dst(mEmptyFileSystem, "a");
  src.copyTo(dst);
  EXPECT_EQ(mFileSystem, src.getFileSystem());
  EXPECT_EQ("", src.getPath());
  EXPECT_EQ(QStringList{"a"}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a.txt"}, mFileSystem->getFiles());
  EXPECT_EQ(QStringList{"b"}, mFileSystem->getDirs("a"));
  EXPECT_EQ(QStringList{"b.txt"}, mFileSystem->getFiles("a"));
  EXPECT_EQ(QStringList{"a"}, mEmptyFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a"}, mEmptyFileSystem->getDirs("a"));
  EXPECT_EQ(QStringList{}, mEmptyFileSystem->getFiles());
  EXPECT_EQ(QStringList{"a.txt"}, mEmptyFileSystem->getFiles("a"));
  EXPECT_EQ(QStringList{"b.txt"}, mEmptyFileSystem->getFiles("a/a"));
}

TEST_F(TransactionalDirectoryTest, testCopyToFromSubdirToRoot) {
  TransactionalDirectory src(mFileSystem, "a");
  TransactionalDirectory dst(mEmptyFileSystem, "");
  src.copyTo(dst);
  EXPECT_EQ(mFileSystem, src.getFileSystem());
  EXPECT_EQ("a", src.getPath());
  EXPECT_EQ(QStringList{"a"}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a.txt"}, mFileSystem->getFiles());
  EXPECT_EQ(QStringList{"b"}, mFileSystem->getDirs("a"));
  EXPECT_EQ(QStringList{"b.txt"}, mFileSystem->getFiles("a"));
  EXPECT_EQ(QStringList{"b"}, mEmptyFileSystem->getDirs());
  EXPECT_EQ(QStringList{"b.txt"}, mEmptyFileSystem->getFiles());
  EXPECT_EQ(QStringList{"c.txt"}, mEmptyFileSystem->getFiles("b"));
}

TEST_F(TransactionalDirectoryTest, testCopyToFromSubdirToSubdir) {
  TransactionalDirectory src(mFileSystem, "a");
  TransactionalDirectory dst(mEmptyFileSystem, "a");
  src.copyTo(dst);
  EXPECT_EQ(mFileSystem, src.getFileSystem());
  EXPECT_EQ("a", src.getPath());
  EXPECT_EQ(QStringList{"a"}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a.txt"}, mFileSystem->getFiles());
  EXPECT_EQ(QStringList{"b"}, mFileSystem->getDirs("a"));
  EXPECT_EQ(QStringList{"b.txt"}, mFileSystem->getFiles("a"));
  EXPECT_EQ(QStringList{"a"}, mEmptyFileSystem->getDirs());
  EXPECT_EQ(QStringList{}, mEmptyFileSystem->getFiles());
  EXPECT_EQ(QStringList{"b"}, mEmptyFileSystem->getDirs("a"));
  EXPECT_EQ(QStringList{"b.txt"}, mEmptyFileSystem->getFiles("a"));
  EXPECT_EQ(QStringList{"c"}, mEmptyFileSystem->getDirs("a/b"));
  EXPECT_EQ(QStringList{"c.txt"}, mEmptyFileSystem->getFiles("a/b"));
}

/*******************************************************************************
 *  Test saveTo()
 ******************************************************************************/

TEST_F(TransactionalDirectoryTest, testSaveToFromRootToRoot) {
  TransactionalDirectory src(mFileSystem, "");
  TransactionalDirectory dst(mEmptyFileSystem, "");
  src.saveTo(dst);
  EXPECT_EQ(mEmptyFileSystem, src.getFileSystem());
  EXPECT_EQ("", src.getPath());
  EXPECT_EQ(QStringList{"a"}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a.txt"}, mFileSystem->getFiles());
  EXPECT_EQ(QStringList{"b"}, mFileSystem->getDirs("a"));
  EXPECT_EQ(QStringList{"b.txt"}, mFileSystem->getFiles("a"));
  EXPECT_EQ(QStringList{"a"}, mEmptyFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a.txt"}, mEmptyFileSystem->getFiles());
  EXPECT_EQ(QStringList{"b.txt"}, mEmptyFileSystem->getFiles("a"));
}

TEST_F(TransactionalDirectoryTest, testSaveToFromRootToSubdir) {
  TransactionalDirectory src(mFileSystem, "");
  TransactionalDirectory dst(mEmptyFileSystem, "a");
  src.saveTo(dst);
  EXPECT_EQ(mEmptyFileSystem, src.getFileSystem());
  EXPECT_EQ("a", src.getPath());
  EXPECT_EQ(QStringList{"a"}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a.txt"}, mFileSystem->getFiles());
  EXPECT_EQ(QStringList{"b"}, mFileSystem->getDirs("a"));
  EXPECT_EQ(QStringList{"b.txt"}, mFileSystem->getFiles("a"));
  EXPECT_EQ(QStringList{"a"}, mEmptyFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a"}, mEmptyFileSystem->getDirs("a"));
  EXPECT_EQ(QStringList{}, mEmptyFileSystem->getFiles());
  EXPECT_EQ(QStringList{"a.txt"}, mEmptyFileSystem->getFiles("a"));
  EXPECT_EQ(QStringList{"b.txt"}, mEmptyFileSystem->getFiles("a/a"));
}

TEST_F(TransactionalDirectoryTest, testSaveToFromSubdirToRoot) {
  TransactionalDirectory src(mFileSystem, "a");
  TransactionalDirectory dst(mEmptyFileSystem, "");
  src.saveTo(dst);
  EXPECT_EQ(mEmptyFileSystem, src.getFileSystem());
  EXPECT_EQ("", src.getPath());
  EXPECT_EQ(QStringList{"a"}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a.txt"}, mFileSystem->getFiles());
  EXPECT_EQ(QStringList{"b"}, mFileSystem->getDirs("a"));
  EXPECT_EQ(QStringList{"b.txt"}, mFileSystem->getFiles("a"));
  EXPECT_EQ(QStringList{"b"}, mEmptyFileSystem->getDirs());
  EXPECT_EQ(QStringList{"b.txt"}, mEmptyFileSystem->getFiles());
  EXPECT_EQ(QStringList{"c.txt"}, mEmptyFileSystem->getFiles("b"));
}

TEST_F(TransactionalDirectoryTest, testSaveToFromSubdirToSubdir) {
  TransactionalDirectory src(mFileSystem, "a");
  TransactionalDirectory dst(mEmptyFileSystem, "a");
  src.saveTo(dst);
  EXPECT_EQ(mEmptyFileSystem, src.getFileSystem());
  EXPECT_EQ("a", src.getPath());
  EXPECT_EQ(QStringList{"a"}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a.txt"}, mFileSystem->getFiles());
  EXPECT_EQ(QStringList{"b"}, mFileSystem->getDirs("a"));
  EXPECT_EQ(QStringList{"b.txt"}, mFileSystem->getFiles("a"));
  EXPECT_EQ(QStringList{"a"}, mEmptyFileSystem->getDirs());
  EXPECT_EQ(QStringList{}, mEmptyFileSystem->getFiles());
  EXPECT_EQ(QStringList{"b"}, mEmptyFileSystem->getDirs("a"));
  EXPECT_EQ(QStringList{"b.txt"}, mEmptyFileSystem->getFiles("a"));
  EXPECT_EQ(QStringList{"c"}, mEmptyFileSystem->getDirs("a/b"));
  EXPECT_EQ(QStringList{"c.txt"}, mEmptyFileSystem->getFiles("a/b"));
}

/*******************************************************************************
 *  Test moveTo()
 ******************************************************************************/

TEST_F(TransactionalDirectoryTest, testMoveToFromRootToRoot) {
  TransactionalDirectory src(mFileSystem, "");
  TransactionalDirectory dst(mEmptyFileSystem, "");
  src.moveTo(dst);
  EXPECT_EQ(mEmptyFileSystem, src.getFileSystem());
  EXPECT_EQ("", src.getPath());
  EXPECT_EQ(QStringList{}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{}, mFileSystem->getFiles());
  EXPECT_EQ(QStringList{"a"}, mEmptyFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a.txt"}, mEmptyFileSystem->getFiles());
  EXPECT_EQ(QStringList{"b.txt"}, mEmptyFileSystem->getFiles("a"));
}

TEST_F(TransactionalDirectoryTest, testMoveToFromRootToSubdir) {
  TransactionalDirectory src(mFileSystem, "");
  TransactionalDirectory dst(mEmptyFileSystem, "a");
  src.moveTo(dst);
  EXPECT_EQ(mEmptyFileSystem, src.getFileSystem());
  EXPECT_EQ("a", src.getPath());
  EXPECT_EQ(QStringList{}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{}, mFileSystem->getFiles());
  EXPECT_EQ(QStringList{"a"}, mEmptyFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a"}, mEmptyFileSystem->getDirs("a"));
  EXPECT_EQ(QStringList{}, mEmptyFileSystem->getFiles());
  EXPECT_EQ(QStringList{"a.txt"}, mEmptyFileSystem->getFiles("a"));
  EXPECT_EQ(QStringList{"b.txt"}, mEmptyFileSystem->getFiles("a/a"));
}

TEST_F(TransactionalDirectoryTest, testMoveToFromSubdirToRoot) {
  TransactionalDirectory src(mFileSystem, "a");
  TransactionalDirectory dst(mEmptyFileSystem, "");
  src.moveTo(dst);
  EXPECT_EQ(mEmptyFileSystem, src.getFileSystem());
  EXPECT_EQ("", src.getPath());
  EXPECT_EQ(QStringList{}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a.txt"}, mFileSystem->getFiles());
  EXPECT_EQ(QStringList{"b"}, mEmptyFileSystem->getDirs());
  EXPECT_EQ(QStringList{"b.txt"}, mEmptyFileSystem->getFiles());
  EXPECT_EQ(QStringList{"c.txt"}, mEmptyFileSystem->getFiles("b"));
}

TEST_F(TransactionalDirectoryTest, testMoveToFromSubdirToSubdir) {
  TransactionalDirectory src(mFileSystem, "a");
  TransactionalDirectory dst(mEmptyFileSystem, "a");
  src.moveTo(dst);
  EXPECT_EQ(mEmptyFileSystem, src.getFileSystem());
  EXPECT_EQ("a", src.getPath());
  EXPECT_EQ(QStringList{}, mFileSystem->getDirs());
  EXPECT_EQ(QStringList{"a.txt"}, mFileSystem->getFiles());
  EXPECT_EQ(QStringList{"a"}, mEmptyFileSystem->getDirs());
  EXPECT_EQ(QStringList{}, mEmptyFileSystem->getFiles());
  EXPECT_EQ(QStringList{"b"}, mEmptyFileSystem->getDirs("a"));
  EXPECT_EQ(QStringList{"b.txt"}, mEmptyFileSystem->getFiles("a"));
  EXPECT_EQ(QStringList{"c"}, mEmptyFileSystem->getDirs("a/b"));
  EXPECT_EQ(QStringList{"c.txt"}, mEmptyFileSystem->getFiles("a/b"));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
