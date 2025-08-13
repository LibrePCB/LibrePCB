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
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/fileio/ziparchive.h>
#include <librepcb/core/utils/toolbox.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class TransactionalFileSystemTest : public ::testing::Test {
protected:
  FilePath mTmpDir;
  FilePath mNonExistingDir;
  FilePath mEmptyDir;
  FilePath mPopulatedDir;

  TransactionalFileSystemTest() {
    // temporary dir (with spaces in path to make tests harder)
    mTmpDir = FilePath::getRandomTempPath().getPathTo("spaces in path");
    FileUtils::writeFile(mTmpDir.getPathTo("1.txt"), "1");

    // non-existing dir
    mNonExistingDir = mTmpDir.getPathTo("nonexisting");

    // empty dir
    mEmptyDir = mTmpDir.getPathTo("empty");
    FileUtils::makePath(mEmptyDir);

    // populated dir
    mPopulatedDir = mTmpDir.getPathTo("populated");
    FileUtils::makePath(mPopulatedDir.getPathTo(".dot/dir"));
    FileUtils::makePath(mPopulatedDir.getPathTo("1/2/3"));
    FileUtils::makePath(mPopulatedDir.getPathTo("a/b"));
    FileUtils::makePath(mPopulatedDir.getPathTo("foo dir/bar dir"));
    FileUtils::writeFile(mPopulatedDir.getPathTo("1.txt"), "1");
    FileUtils::writeFile(mPopulatedDir.getPathTo("2.txt"), "2");
    FileUtils::writeFile(mPopulatedDir.getPathTo(".dot/file.txt"), "file");
    FileUtils::writeFile(mPopulatedDir.getPathTo(".dot/dir/foo.txt"), "foo");
    FileUtils::writeFile(mPopulatedDir.getPathTo("1/1a.txt"), "1a");
    FileUtils::writeFile(mPopulatedDir.getPathTo("1/1b.txt"), "1b");
    FileUtils::writeFile(mPopulatedDir.getPathTo("1/2/3/4.txt"), "4");
    FileUtils::writeFile(mPopulatedDir.getPathTo("a/b/c"), "c");
    FileUtils::writeFile(mPopulatedDir.getPathTo("foo dir/bar dir.txt"), "bar");
    FileUtils::writeFile(mPopulatedDir.getPathTo("foo dir/bar dir/X"), "X");
  }

  virtual ~TransactionalFileSystemTest() {
    QDir(mTmpDir.toStr()).removeRecursively();
  }
};

/*******************************************************************************
 *  Non-Parametrized Tests
 ******************************************************************************/

TEST_F(TransactionalFileSystemTest, testConstructorNonExistingDir) {
  TransactionalFileSystem fs(mNonExistingDir, true);
}

TEST_F(TransactionalFileSystemTest, testConstructorEmptyDir) {
  TransactionalFileSystem fs(mEmptyDir, true);
}

TEST_F(TransactionalFileSystemTest, testConstructorPopulatedDir) {
  TransactionalFileSystem fs(mPopulatedDir, true);
}

TEST_F(TransactionalFileSystemTest, testGetPath) {
  TransactionalFileSystem fs(mPopulatedDir, false);
  EXPECT_EQ(mPopulatedDir, fs.getPath());
}

TEST_F(TransactionalFileSystemTest, testIsWritableFalse) {
  TransactionalFileSystem fs(mPopulatedDir, false);
  EXPECT_FALSE(fs.isWritable());
}

TEST_F(TransactionalFileSystemTest, testIsWritableTrue) {
  TransactionalFileSystem fs(mPopulatedDir, true);
  EXPECT_TRUE(fs.isWritable());
}

TEST_F(TransactionalFileSystemTest, testGetAbsPathWithoutArgument) {
  TransactionalFileSystem fs(mPopulatedDir);
  EXPECT_EQ(mPopulatedDir, fs.getAbsPath());
}

TEST_F(TransactionalFileSystemTest, testGetAbsPathWithArgument) {
  TransactionalFileSystem fs(mPopulatedDir);
  EXPECT_EQ(mPopulatedDir.getPathTo("foo/bar"), fs.getAbsPath("foo/bar"));
}

TEST_F(TransactionalFileSystemTest, testWriteCreatesNewFile) {
  TransactionalFileSystem fs(mPopulatedDir, true);
  ASSERT_FALSE(fs.fileExists("new file"));
  fs.write("new file", "content");
  EXPECT_TRUE(fs.fileExists("new file"));
  EXPECT_EQ("content", fs.read("new file"));
}

TEST_F(TransactionalFileSystemTest, testWriteExistingFile) {
  TransactionalFileSystem fs(mPopulatedDir, true);
  ASSERT_TRUE(fs.fileExists("1.txt"));
  ASSERT_EQ("1", fs.read("1.txt"));
  fs.write("1.txt", "new content");
  EXPECT_TRUE(fs.fileExists("1.txt"));
  EXPECT_EQ("new content", fs.read("1.txt"));
}

TEST_F(TransactionalFileSystemTest, testWriteCreatesNewDirectoryAndFile) {
  TransactionalFileSystem fs(mPopulatedDir, true);
  ASSERT_FALSE(fs.fileExists("x/y/z"));
  fs.write("x/y/z", "foo");
  EXPECT_TRUE(fs.fileExists("x/y/z"));
  EXPECT_TRUE(fs.getDirs().contains("x"));
  EXPECT_TRUE(fs.getDirs("x").contains("y"));
  EXPECT_TRUE(fs.getFiles("x/y").contains("z"));
}

TEST_F(TransactionalFileSystemTest, testWriteIsDelayedUntilSave) {
  FilePath fp = mPopulatedDir.getPathTo("new dir/new file");
  TransactionalFileSystem fs(mPopulatedDir, true);
  ASSERT_FALSE(fs.fileExists(fp.toRelative(mPopulatedDir)));
  ASSERT_FALSE(fp.isExistingFile());

  // write file
  fs.write(fp.toRelative(mPopulatedDir), "content");
  EXPECT_FALSE(fp.isExistingFile());

  // save
  fs.save();
  EXPECT_TRUE(fp.isExistingFile());
  EXPECT_EQ("content", FileUtils::readFile(fp));
}

TEST_F(TransactionalFileSystemTest, testRemoveExistingFile) {
  FilePath fp = mPopulatedDir.getPathTo("1/1a.txt");
  TransactionalFileSystem fs(mPopulatedDir, true);
  ASSERT_TRUE(fs.fileExists(fp.toRelative(mPopulatedDir)));
  ASSERT_TRUE(fs.getFiles("1").contains("1a.txt"));
  ASSERT_TRUE(fp.isExistingFile());

  // remove file
  fs.removeFile(fp.toRelative(mPopulatedDir));
  EXPECT_FALSE(fs.fileExists(fp.toRelative(mPopulatedDir)));
  EXPECT_FALSE(fs.getFiles("1").contains("1a.txt"));
  EXPECT_TRUE(fp.isExistingFile());

  // save
  fs.save();
  EXPECT_FALSE(fs.fileExists(fp.toRelative(mPopulatedDir)));
  EXPECT_FALSE(fs.getFiles("1").contains("1a.txt"));
  EXPECT_FALSE(fp.isExistingFile());
}

TEST_F(TransactionalFileSystemTest, testRemoveNewFile) {
  FilePath fp = mPopulatedDir.getPathTo("1/nonexisting.txt");
  TransactionalFileSystem fs(mPopulatedDir, true);
  ASSERT_FALSE(fs.fileExists(fp.toRelative(mPopulatedDir)));
  ASSERT_FALSE(fs.getFiles("1").contains("nonexisting.txt"));
  ASSERT_FALSE(fp.isExistingFile());

  // create new file
  fs.write(fp.toRelative(mPopulatedDir), "foo");
  EXPECT_TRUE(fs.fileExists(fp.toRelative(mPopulatedDir)));
  EXPECT_TRUE(fs.getFiles("1").contains("nonexisting.txt"));
  EXPECT_FALSE(fp.isExistingFile());

  // remove the new file
  fs.removeFile(fp.toRelative(mPopulatedDir));
  EXPECT_FALSE(fs.fileExists(fp.toRelative(mPopulatedDir)));
  EXPECT_FALSE(fs.getFiles("1").contains("nonexisting.txt"));
  EXPECT_FALSE(fp.isExistingFile());

  // save
  fs.save();
  EXPECT_FALSE(fs.fileExists(fp.toRelative(mPopulatedDir)));
  EXPECT_FALSE(fs.getFiles("1").contains("nonexisting.txt"));
  EXPECT_FALSE(fp.isExistingFile());
}

TEST_F(TransactionalFileSystemTest, testRemoveDirRecursively) {
  FilePath dp = mPopulatedDir.getPathTo(".dot");
  FilePath fp = mPopulatedDir.getPathTo(".dot/dir/foo.txt");
  TransactionalFileSystem fs(mPopulatedDir, true);
  ASSERT_TRUE(fs.fileExists(fp.toRelative(mPopulatedDir)));
  ASSERT_TRUE(fs.getDirs().contains(".dot"));
  ASSERT_TRUE(fs.getDirs(".dot").contains("dir"));
  ASSERT_TRUE(fs.getFiles(".dot/dir").contains("foo.txt"));
  ASSERT_TRUE(dp.isExistingDir());
  ASSERT_TRUE(fp.isExistingFile());

  // remove dir
  fs.removeDirRecursively(".dot");
  EXPECT_FALSE(fs.fileExists(fp.toRelative(mPopulatedDir)));
  EXPECT_FALSE(fs.getDirs().contains(".dot"));
  EXPECT_FALSE(fs.getDirs(".dot").contains("dir"));
  EXPECT_FALSE(fs.getFiles(".dot/dir").contains("foo.txt"));
  EXPECT_TRUE(dp.isExistingDir());
  EXPECT_TRUE(fp.isExistingFile());

  // save
  fs.save();
  EXPECT_FALSE(fs.fileExists(fp.toRelative(mPopulatedDir)));
  EXPECT_FALSE(fs.getDirs().contains(".dot"));
  EXPECT_FALSE(fs.getDirs(".dot").contains("dir"));
  EXPECT_FALSE(fs.getFiles(".dot/dir").contains("foo.txt"));
  EXPECT_FALSE(dp.isExistingDir());
  EXPECT_FALSE(fp.isExistingFile());
}

TEST_F(TransactionalFileSystemTest, testRemoveSubDirRecursively) {
  FilePath dp = mPopulatedDir.getPathTo(".dot");
  FilePath sp = mPopulatedDir.getPathTo(".dot/dir");
  FilePath fp = mPopulatedDir.getPathTo(".dot/dir/foo.txt");
  TransactionalFileSystem fs(mPopulatedDir, true);
  ASSERT_TRUE(fs.fileExists(fp.toRelative(mPopulatedDir)));
  ASSERT_TRUE(fs.getDirs().contains(".dot"));
  ASSERT_TRUE(fs.getDirs(".dot").contains("dir"));
  ASSERT_TRUE(fs.getFiles(".dot/dir").contains("foo.txt"));
  ASSERT_TRUE(dp.isExistingDir());
  ASSERT_TRUE(sp.isExistingDir());
  ASSERT_TRUE(fp.isExistingFile());

  // remove dir
  fs.removeDirRecursively(".dot/dir");
  EXPECT_FALSE(fs.fileExists(fp.toRelative(mPopulatedDir)));
  EXPECT_TRUE(fs.getDirs().contains(".dot"));
  EXPECT_FALSE(fs.getDirs(".dot").contains("dir"));
  EXPECT_FALSE(fs.getFiles(".dot/dir").contains("foo.txt"));
  EXPECT_TRUE(dp.isExistingDir());
  EXPECT_TRUE(sp.isExistingDir());
  EXPECT_TRUE(fp.isExistingFile());

  // save
  fs.save();
  EXPECT_FALSE(fs.fileExists(fp.toRelative(mPopulatedDir)));
  EXPECT_TRUE(fs.getDirs().contains(".dot"));
  EXPECT_FALSE(fs.getDirs(".dot").contains("dir"));
  EXPECT_FALSE(fs.getFiles(".dot/dir").contains("foo.txt"));
  EXPECT_TRUE(dp.isExistingDir());
  EXPECT_FALSE(sp.isExistingDir());
  EXPECT_FALSE(fp.isExistingFile());
}

TEST_F(TransactionalFileSystemTest, testSaveThrowsExceptionIfNonWritable) {
  TransactionalFileSystem fs(mPopulatedDir, false);
  EXPECT_THROW(fs.save(), Exception);
}

TEST_F(TransactionalFileSystemTest, testCombinationOfAllMethods) {
  TransactionalFileSystem fs(mPopulatedDir, true);

  // check initial state
  ASSERT_FALSE(fs.fileExists("x/y/z"));
  ASSERT_FALSE(fs.fileExists("z/y/x.txt"));
  ASSERT_FALSE(fs.fileExists("z/y.txt"));
  ASSERT_TRUE(fs.fileExists("1.txt"));
  ASSERT_TRUE(fs.fileExists("a/b/c"));
  ASSERT_FALSE(fs.fileExists("z/1.txt"));
  ASSERT_FALSE(fs.fileExists("z/2.txt"));

  // do some file operations
  fs.write("x/y/z", "z");  // create new file
  fs.write("z/y/x.txt", "x");  // create new file
  fs.write("z/y.txt", "y");  // create new file
  fs.write("1.txt", "new 1");  // overwrite existing file
  fs.write(".dot/file.txt", "new file");  // overwrite existing file
  fs.removeFile("z/y/x.txt");  // remove new file
  fs.removeFile("1.txt");  // remove existing file
  fs.removeDirRecursively("z");  // remove new directory
  fs.removeDirRecursively("a");  // remove existing directory
  fs.write("z/1.txt", "1");  // create new file
  fs.write("z/2.txt", "2");  // create new file
  fs.removeFile("z/1.txt");  // remove new file

  // check state in memory
  EXPECT_TRUE(fs.fileExists("x/y/z"));
  EXPECT_FALSE(fs.fileExists("z/y/x.txt"));
  EXPECT_FALSE(fs.fileExists("z/y.txt"));
  EXPECT_FALSE(fs.fileExists("1.txt"));
  EXPECT_FALSE(fs.fileExists("a/b/c"));
  EXPECT_FALSE(fs.fileExists("z/1.txt"));
  EXPECT_TRUE(fs.fileExists("z/2.txt"));
  EXPECT_EQ("z", fs.read("x/y/z"));
  EXPECT_EQ("2", fs.read("z/2.txt"));
  EXPECT_EQ("new file", fs.read(".dot/file.txt"));
  EXPECT_THROW(fs.read("z/y/x.txt"), Exception);
  EXPECT_THROW(fs.read("z/y.txt"), Exception);
  EXPECT_THROW(fs.read("1.txt"), Exception);
  EXPECT_THROW(fs.read("a/b/c"), Exception);
  EXPECT_THROW(fs.read("z/1.txt"), Exception);

  // check state on file system
  EXPECT_FALSE(fs.getAbsPath("x/y/z").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/y/x.txt").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/y.txt").isExistingFile());
  EXPECT_TRUE(fs.getAbsPath("1.txt").isExistingFile());
  EXPECT_TRUE(fs.getAbsPath("a/b/c").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/1.txt").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/2.txt").isExistingFile());
  EXPECT_EQ("1", FileUtils::readFile(fs.getAbsPath("1.txt")));
  EXPECT_EQ("c", FileUtils::readFile(fs.getAbsPath("a/b/c")));
  EXPECT_EQ("file", FileUtils::readFile(fs.getAbsPath(".dot/file.txt")));

  // save to file system
  fs.save();

  // check state in memory (equal to the state before saving)
  EXPECT_TRUE(fs.fileExists("x/y/z"));
  EXPECT_FALSE(fs.fileExists("z/y/x.txt"));
  EXPECT_FALSE(fs.fileExists("z/y.txt"));
  EXPECT_FALSE(fs.fileExists("1.txt"));
  EXPECT_FALSE(fs.fileExists("a/b/c"));
  EXPECT_FALSE(fs.fileExists("z/1.txt"));
  EXPECT_TRUE(fs.fileExists("z/2.txt"));
  EXPECT_EQ("z", fs.read("x/y/z"));
  EXPECT_EQ("2", fs.read("z/2.txt"));
  EXPECT_EQ("new file", fs.read(".dot/file.txt"));
  EXPECT_THROW(fs.read("z/y/x.txt"), Exception);
  EXPECT_THROW(fs.read("z/y.txt"), Exception);
  EXPECT_THROW(fs.read("1.txt"), Exception);
  EXPECT_THROW(fs.read("a/b/c"), Exception);
  EXPECT_THROW(fs.read("z/1.txt"), Exception);

  // check state on file system
  EXPECT_TRUE(fs.getAbsPath("x/y/z").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/y/x.txt").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/y.txt").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("1.txt").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("a").isExistingDir());
  EXPECT_FALSE(fs.getAbsPath("z/1.txt").isExistingFile());
  EXPECT_TRUE(fs.getAbsPath("z/2.txt").isExistingFile());
  EXPECT_EQ("z", FileUtils::readFile(fs.getAbsPath("x/y/z")));
  EXPECT_EQ("2", FileUtils::readFile(fs.getAbsPath("z/2.txt")));
  EXPECT_EQ("new file", FileUtils::readFile(fs.getAbsPath(".dot/file.txt")));

  // do some more file operations
  fs.write("foo", "foo");  // create new file
  fs.write("z/2.txt", "new 2");  // overwrite existing file
  fs.removeFile("x/y/z");  // remove existing file

  // check state in memory
  EXPECT_FALSE(fs.fileExists("x/y/z"));
  EXPECT_FALSE(fs.fileExists("z/y/x.txt"));
  EXPECT_FALSE(fs.fileExists("z/y.txt"));
  EXPECT_FALSE(fs.fileExists("1.txt"));
  EXPECT_FALSE(fs.fileExists("a/b/c"));
  EXPECT_FALSE(fs.fileExists("z/1.txt"));
  EXPECT_TRUE(fs.fileExists("z/2.txt"));
  EXPECT_TRUE(fs.fileExists("foo"));
  EXPECT_EQ("new 2", fs.read("z/2.txt"));
  EXPECT_EQ("foo", fs.read("foo"));
  EXPECT_THROW(fs.read("x/y/z"), Exception);
  EXPECT_THROW(fs.read("z/y/x.txt"), Exception);
  EXPECT_THROW(fs.read("z/y.txt"), Exception);
  EXPECT_THROW(fs.read("1.txt"), Exception);
  EXPECT_THROW(fs.read("a/b/c"), Exception);
  EXPECT_THROW(fs.read("z/1.txt"), Exception);

  // save to file system
  fs.save();

  // check state in memory (equal to the state before saving)
  EXPECT_FALSE(fs.fileExists("x/y/z"));
  EXPECT_FALSE(fs.fileExists("z/y/x.txt"));
  EXPECT_FALSE(fs.fileExists("z/y.txt"));
  EXPECT_FALSE(fs.fileExists("1.txt"));
  EXPECT_FALSE(fs.fileExists("a/b/c"));
  EXPECT_FALSE(fs.fileExists("z/1.txt"));
  EXPECT_TRUE(fs.fileExists("z/2.txt"));
  EXPECT_TRUE(fs.fileExists("foo"));
  EXPECT_EQ("new 2", fs.read("z/2.txt"));
  EXPECT_EQ("foo", fs.read("foo"));
  EXPECT_THROW(fs.read("x/y/z"), Exception);
  EXPECT_THROW(fs.read("z/y/x.txt"), Exception);
  EXPECT_THROW(fs.read("z/y.txt"), Exception);
  EXPECT_THROW(fs.read("1.txt"), Exception);
  EXPECT_THROW(fs.read("a/b/c"), Exception);
  EXPECT_THROW(fs.read("z/1.txt"), Exception);

  // check state on file system
  EXPECT_FALSE(fs.getAbsPath("x/y/z").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/y/x.txt").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/y.txt").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("1.txt").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("a").isExistingDir());
  EXPECT_FALSE(fs.getAbsPath("z/1.txt").isExistingFile());
  EXPECT_TRUE(fs.getAbsPath("z/2.txt").isExistingFile());
  EXPECT_TRUE(fs.getAbsPath("foo").isExistingFile());
  EXPECT_EQ("new 2", FileUtils::readFile(fs.getAbsPath("z/2.txt")));
  EXPECT_EQ("foo", FileUtils::readFile(fs.getAbsPath("foo")));
  EXPECT_EQ("new file", FileUtils::readFile(fs.getAbsPath(".dot/file.txt")));
}

TEST_F(TransactionalFileSystemTest, testAutosaveIsRemovedWhenSaving) {
  FilePath fp = mPopulatedDir.getPathTo(".autosave");
  TransactionalFileSystem fs(mPopulatedDir, true);
  fs.autosave();
  ASSERT_TRUE(fp.isExistingDir());
  fs.save();
  EXPECT_FALSE(fp.isExistingDir());
}

TEST_F(TransactionalFileSystemTest, testAutosaveIsRemovedInDestructor) {
  FilePath fp = mPopulatedDir.getPathTo(".autosave");
  {
    TransactionalFileSystem fs(mPopulatedDir, true);
    fs.autosave();
    ASSERT_TRUE(fp.isExistingDir());
  }
  EXPECT_FALSE(fp.isExistingDir());
}

TEST_F(TransactionalFileSystemTest, testRestoreAutosave) {
  TransactionalFileSystem fs(mPopulatedDir, true);

  // check initial state
  ASSERT_FALSE(fs.fileExists("x/y/z"));
  ASSERT_FALSE(fs.fileExists("z/y/x.txt"));
  ASSERT_FALSE(fs.fileExists("z/y.txt"));
  ASSERT_TRUE(fs.fileExists("1.txt"));
  ASSERT_TRUE(fs.fileExists("a/b/c"));
  ASSERT_FALSE(fs.fileExists("z/1.txt"));
  ASSERT_FALSE(fs.fileExists("z/2.txt"));

  // do some file operations
  fs.write("x/y/z", "z");  // create new file
  fs.write("z/y/x.txt", "x");  // create new file
  fs.write("z/y.txt", "y");  // create new file
  fs.write("1.txt", "new 1");  // overwrite existing file
  fs.write(".dot/file.txt", "new file");  // overwrite existing file
  fs.removeFile("z/y/x.txt");  // remove new file
  fs.removeFile("1.txt");  // remove existing file
  fs.removeDirRecursively("z");  // remove new directory
  fs.removeDirRecursively("a");  // remove existing directory
  fs.write("z/1.txt", "1");  // create new file
  fs.write("z/2.txt", "2");  // create new file
  fs.removeFile("z/1.txt");  // remove new file

  // check state in memory
  EXPECT_TRUE(fs.fileExists("x/y/z"));
  EXPECT_FALSE(fs.fileExists("z/y/x.txt"));
  EXPECT_FALSE(fs.fileExists("z/y.txt"));
  EXPECT_FALSE(fs.fileExists("1.txt"));
  EXPECT_FALSE(fs.fileExists("a/b/c"));
  EXPECT_FALSE(fs.fileExists("z/1.txt"));
  EXPECT_TRUE(fs.fileExists("z/2.txt"));
  EXPECT_EQ("z", fs.read("x/y/z"));
  EXPECT_EQ("2", fs.read("z/2.txt"));
  EXPECT_EQ("new file", fs.read(".dot/file.txt"));
  EXPECT_THROW(fs.read("z/y/x.txt"), Exception);
  EXPECT_THROW(fs.read("z/y.txt"), Exception);
  EXPECT_THROW(fs.read("1.txt"), Exception);
  EXPECT_THROW(fs.read("a/b/c"), Exception);
  EXPECT_THROW(fs.read("z/1.txt"), Exception);

  // check state on file system
  EXPECT_FALSE(fs.getAbsPath("x/y/z").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/y/x.txt").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/y.txt").isExistingFile());
  EXPECT_TRUE(fs.getAbsPath("1.txt").isExistingFile());
  EXPECT_TRUE(fs.getAbsPath("a/b/c").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/1.txt").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/2.txt").isExistingFile());
  EXPECT_EQ("1", FileUtils::readFile(fs.getAbsPath("1.txt")));
  EXPECT_EQ("c", FileUtils::readFile(fs.getAbsPath("a/b/c")));
  EXPECT_EQ("file", FileUtils::readFile(fs.getAbsPath(".dot/file.txt")));

  // perform autosave
  fs.autosave();

  // remove lock because we can't get a stale lock without crashing the app
  FileUtils::removeFile(mPopulatedDir.getPathTo(".lock"));

  // open another file system on the same directory to restore the autosave
  TransactionalFileSystem fs2(mPopulatedDir, true,
                              &TransactionalFileSystem::RestoreMode::yes);
  EXPECT_TRUE(fs2.isRestoredFromAutosave());

  // check state in memory
  EXPECT_TRUE(fs2.fileExists("x/y/z"));
  EXPECT_FALSE(fs2.fileExists("z/y/x.txt"));
  EXPECT_FALSE(fs2.fileExists("z/y.txt"));
  EXPECT_FALSE(fs2.fileExists("1.txt"));
  EXPECT_FALSE(fs2.fileExists("a/b/c"));
  EXPECT_FALSE(fs2.fileExists("z/1.txt"));
  EXPECT_TRUE(fs2.fileExists("z/2.txt"));
  EXPECT_EQ("z", fs2.read("x/y/z"));
  EXPECT_EQ("2", fs2.read("z/2.txt"));
  EXPECT_EQ("new file", fs2.read(".dot/file.txt"));
  EXPECT_THROW(fs2.read("z/y/x.txt"), Exception);
  EXPECT_THROW(fs2.read("z/y.txt"), Exception);
  EXPECT_THROW(fs2.read("1.txt"), Exception);
  EXPECT_THROW(fs2.read("a/b/c"), Exception);
  EXPECT_THROW(fs2.read("z/1.txt"), Exception);

  // check state on file system
  EXPECT_FALSE(fs2.getAbsPath("x/y/z").isExistingFile());
  EXPECT_FALSE(fs2.getAbsPath("z/y/x.txt").isExistingFile());
  EXPECT_FALSE(fs2.getAbsPath("z/y.txt").isExistingFile());
  EXPECT_TRUE(fs2.getAbsPath("1.txt").isExistingFile());
  EXPECT_TRUE(fs2.getAbsPath("a/b/c").isExistingFile());
  EXPECT_FALSE(fs2.getAbsPath("z/1.txt").isExistingFile());
  EXPECT_FALSE(fs2.getAbsPath("z/2.txt").isExistingFile());
  EXPECT_EQ("1", FileUtils::readFile(fs2.getAbsPath("1.txt")));
  EXPECT_EQ("c", FileUtils::readFile(fs2.getAbsPath("a/b/c")));
  EXPECT_EQ("file", FileUtils::readFile(fs2.getAbsPath(".dot/file.txt")));

  // save to file system
  fs2.save();

  // check state on file system
  EXPECT_TRUE(fs2.getAbsPath("x/y/z").isExistingFile());
  EXPECT_FALSE(fs2.getAbsPath("z/y/x.txt").isExistingFile());
  EXPECT_FALSE(fs2.getAbsPath("z/y.txt").isExistingFile());
  EXPECT_FALSE(fs2.getAbsPath("1.txt").isExistingFile());
  EXPECT_FALSE(fs2.getAbsPath("a").isExistingDir());
  EXPECT_FALSE(fs2.getAbsPath("z/1.txt").isExistingFile());
  EXPECT_TRUE(fs2.getAbsPath("z/2.txt").isExistingFile());
  EXPECT_EQ("z", FileUtils::readFile(fs2.getAbsPath("x/y/z")));
  EXPECT_EQ("2", FileUtils::readFile(fs2.getAbsPath("z/2.txt")));
  EXPECT_EQ("new file", FileUtils::readFile(fs2.getAbsPath(".dot/file.txt")));
}

TEST_F(TransactionalFileSystemTest, testRestoredBackupAfterFailedSave) {
  FilePath backupDir = mPopulatedDir.getPathTo(".backup");

  {
    TransactionalFileSystem fs(mPopulatedDir, true);
    fs.write("x/y/z", "z");  // create new file
    fs.write("1.txt", "new 1");  // overwrite existing file
    fs.removeFile("2.txt");  // remove existing file
    fs.removeDirRecursively("a");  // remove existing directory

    // create a directory where x/y/z would be saved to -> leads to an error
    // when saving the file system.
    FileUtils::makePath(mPopulatedDir.getPathTo("x/y/z"));

    // save must now fail and the ".backup" directory must persist
    EXPECT_THROW(fs.save(), Exception);
    EXPECT_TRUE(backupDir.isExistingDir());
  }

  for (int i = 0; i < 2; ++i) {
    // opening the file system must automatically restore the backup
    TransactionalFileSystem fs(mPopulatedDir, true);
    EXPECT_EQ("z", fs.read("x/y/z"));
    EXPECT_EQ("new 1", fs.read("1.txt"));
    EXPECT_FALSE(fs.fileExists("2.txt"));
    EXPECT_FALSE(fs.getDirs().contains("a"));
    EXPECT_TRUE(backupDir.isExistingDir());
  }

  {
    // Remove the directory now, save file system and the backup must be removed
    FileUtils::removeDirRecursively(mPopulatedDir.getPathTo("x/y/z"));
    TransactionalFileSystem fs(mPopulatedDir, true);
    fs.save();
    EXPECT_FALSE(backupDir.isExistingDir());
  }

  // check if files are written to disk
  EXPECT_EQ("z", FileUtils::readFile(mPopulatedDir.getPathTo("x/y/z")));
  EXPECT_EQ("new 1", FileUtils::readFile(mPopulatedDir.getPathTo("1.txt")));
  EXPECT_FALSE(mPopulatedDir.getPathTo("2.txt").isExistingFile());
  EXPECT_FALSE(mPopulatedDir.getPathTo("a").isExistingDir());
  EXPECT_FALSE(backupDir.isExistingDir());
}

TEST_F(TransactionalFileSystemTest, testExportImportZipByFilePath) {
  FilePath zipFp = mPopulatedDir.getPathTo("export to.zip");
  ASSERT_FALSE(zipFp.isExistingFile());
  {
    TransactionalFileSystem fs(mPopulatedDir, true);
    fs.exportToZip(zipFp);
    EXPECT_TRUE(zipFp.isExistingFile());
  }
  {
    TransactionalFileSystem fs(mEmptyDir, true);
    fs.loadFromZip(zipFp);
    EXPECT_EQ("bar", fs.read("foo dir/bar dir.txt"));
  }
}

TEST_F(TransactionalFileSystemTest, testExportZipByFilePathWithFilter) {
  FilePath zipFp = mPopulatedDir.getPathTo("export to filter.zip");
  TransactionalFileSystem fs(mPopulatedDir, true);
  auto filter = [](const QString& fp) {
    return (fp == "1.txt") || (fp == "1/1a.txt");
  };
  fs.exportToZip(zipFp, filter);

  ZipArchive zip(zipFp);
  EXPECT_EQ(2, zip.getEntriesCount());
}

TEST_F(TransactionalFileSystemTest, testExportImportZipByByteArray) {
  QByteArray content;
  {
    TransactionalFileSystem fs(mPopulatedDir, true);
    content = fs.exportToZip();
  }
  {
    TransactionalFileSystem fs(mEmptyDir, true);
    fs.loadFromZip(content);
    EXPECT_EQ("bar", fs.read("foo dir/bar dir.txt"));
  }
}

TEST_F(TransactionalFileSystemTest, testExportZipByByteArrayWithFilter) {
  TransactionalFileSystem fs(mPopulatedDir, true);
  auto filter = [](const QString& fp) {
    return (fp == "1.txt") || (fp == "1/1a.txt");
  };
  QByteArray content = fs.exportToZip(filter);

  ZipArchive zip(content);
  EXPECT_EQ(2, zip.getEntriesCount());
}

TEST_F(TransactionalFileSystemTest, testDiscardChanges) {
  TransactionalFileSystem fs(mPopulatedDir, true);

  // check initial state
  ASSERT_FALSE(fs.fileExists("x/y/z"));
  ASSERT_FALSE(fs.fileExists("z/y/x.txt"));
  ASSERT_FALSE(fs.fileExists("z/y.txt"));
  ASSERT_TRUE(fs.fileExists("1.txt"));
  ASSERT_TRUE(fs.fileExists("a/b/c"));
  ASSERT_FALSE(fs.fileExists("z/1.txt"));
  ASSERT_FALSE(fs.fileExists("z/2.txt"));

  // do some file operations
  fs.write("x/y/z", "z");  // create new file
  fs.write("z/y/x.txt", "x");  // create new file
  fs.write("z/y.txt", "y");  // create new file
  fs.write("1.txt", "new 1");  // overwrite existing file
  fs.write(".dot/file.txt", "new file");  // overwrite existing file
  fs.removeFile("z/y/x.txt");  // remove new file
  fs.removeFile("1.txt");  // remove existing file
  fs.removeDirRecursively("z");  // remove new directory
  fs.removeDirRecursively("a");  // remove existing directory
  fs.write("z/1.txt", "1");  // create new file
  fs.write("z/2.txt", "2");  // create new file
  fs.removeFile("z/1.txt");  // remove new file

  // discard all changes
  fs.discardChanges();

  // check state in memory
  EXPECT_FALSE(fs.fileExists("x/y/z"));
  EXPECT_FALSE(fs.fileExists("z/y/x.txt"));
  EXPECT_FALSE(fs.fileExists("z/y.txt"));
  EXPECT_TRUE(fs.fileExists("1.txt"));
  EXPECT_TRUE(fs.fileExists("a/b/c"));
  EXPECT_FALSE(fs.fileExists("z/1.txt"));
  EXPECT_FALSE(fs.fileExists("z/2.txt"));

  // save to file system
  fs.save();

  // check state on file system
  EXPECT_FALSE(fs.getAbsPath("x/y/z").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/y/x.txt").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/y.txt").isExistingFile());
  EXPECT_TRUE(fs.getAbsPath("1.txt").isExistingFile());
  EXPECT_TRUE(fs.getAbsPath("a/b/c").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/1.txt").isExistingFile());
  EXPECT_FALSE(fs.getAbsPath("z/2.txt").isExistingFile());
  EXPECT_EQ("1", FileUtils::readFile(fs.getAbsPath("1.txt")));
  EXPECT_EQ("c", FileUtils::readFile(fs.getAbsPath("a/b/c")));
  EXPECT_EQ("file", FileUtils::readFile(fs.getAbsPath(".dot/file.txt")));
}

TEST_F(TransactionalFileSystemTest, testCheckForModifications) {
  TransactionalFileSystem fs(mPopulatedDir, true);

  // check initial state
  ASSERT_FALSE(fs.fileExists("x/y/z"));
  ASSERT_FALSE(fs.fileExists("z/y/x.txt"));
  ASSERT_FALSE(fs.fileExists("z/y.txt"));
  ASSERT_TRUE(fs.fileExists("1.txt"));
  ASSERT_TRUE(fs.fileExists("a/b/c"));
  ASSERT_FALSE(fs.fileExists("z/1.txt"));
  ASSERT_FALSE(fs.fileExists("z/2.txt"));

  // do some file operations
  fs.write("x/y/z", "z");  // create new file
  fs.write("z/y/x.txt", "x");  // create new file
  fs.write("z/y.txt", "y");  // create new file
  fs.write("1.txt", "new 1");  // overwrite existing file
  fs.write(".dot/file.txt", "new file");  // overwrite existing file
  fs.removeFile("z/y/x.txt");  // remove new file
  fs.removeFile("1.txt");  // remove existing file
  fs.removeDirRecursively("z");  // remove new directory
  fs.removeDirRecursively("a");  // remove existing directory
  fs.write("z/1.txt", "1");  // create new file
  fs.write("z/2.txt", "2");  // create new file
  fs.removeFile("z/1.txt");  // remove new file

  // check modifications
  std::list<std::string> modified;
  foreach (const QString& str, fs.checkForModifications()) {
    modified.push_back(str.toStdString());
  }
  modified.sort();
  std::list<std::string> expected = {".dot/file.txt", "1.txt", "a/", "x/y/z",
                                     "z/2.txt"};
  EXPECT_EQ(expected, modified);

  // save to file system
  fs.save();

  // check modifications, should be empty now
  EXPECT_EQ(0, fs.checkForModifications().count());
}

TEST_F(TransactionalFileSystemTest, testReleaseLock) {
  const FilePath lockFp = mPopulatedDir.getPathTo(".lock");

  TransactionalFileSystem fs(mPopulatedDir, true);
  EXPECT_TRUE(lockFp.isExistingFile());
  fs.write("foo", "x");  // Create new file.
  fs.save();
  fs.write("bar", "x");  // Create new file.
  fs.releaseLock();
  EXPECT_FALSE(lockFp.isExistingFile());
  fs.releaseLock();  // Second call should do nothing.
  EXPECT_FALSE(lockFp.isExistingFile());
  fs.write("foobar", "x");  // Create new file.
  EXPECT_THROW(fs.save(), Exception);  // Failed because it's read-only.
}

/*******************************************************************************
 *  Security Tests: Sandbox Breakout
 ******************************************************************************/

// These tests make sure that any file operation outside the file system (i.e.
// with too many "../" in the path) will fail. This is important for security
// reasons (sandbox breakout).

TEST_F(TransactionalFileSystemTest, testGetAbsPathBreakout) {
  TransactionalFileSystem fs(mPopulatedDir);
  EXPECT_FALSE(fs.getAbsPath("../1.txt").isValid());
}

TEST_F(TransactionalFileSystemTest, testGetDirsBreakout) {
  TransactionalFileSystem fs(mPopulatedDir);
  ASSERT_GT(FileUtils::findDirectories(mTmpDir).count(), 0);
  EXPECT_EQ(0, fs.getDirs("../").count());
}

TEST_F(TransactionalFileSystemTest, testGetFilesBreakout) {
  TransactionalFileSystem fs(mPopulatedDir);
  ASSERT_GT(FileUtils::getFilesInDirectory(mTmpDir).count(), 0);
  EXPECT_EQ(0, fs.getFiles("../").count());
}

TEST_F(TransactionalFileSystemTest, testFileExistsBreakout) {
  TransactionalFileSystem fs(mPopulatedDir);
  ASSERT_TRUE(mPopulatedDir.getParentDir().getPathTo("1.txt").isExistingFile());
  EXPECT_FALSE(fs.fileExists("../1.txt"));
}

TEST_F(TransactionalFileSystemTest, testReadBreakout) {
  TransactionalFileSystem fs(mPopulatedDir);
  ASSERT_EQ(
      "1",
      FileUtils::readFile(mPopulatedDir.getParentDir().getPathTo("1.txt")));
  ASSERT_EQ("1", fs.read("1.txt"));
  EXPECT_THROW(fs.read("../1.txt"), Exception);
  EXPECT_THROW(fs.read("../populated/1.txt"), Exception);
  EXPECT_THROW(fs.readIfExists("../1.txt"), Exception);
  EXPECT_THROW(fs.readIfExists("../populated/1.txt"), Exception);
}

TEST_F(TransactionalFileSystemTest, testWriteBreakout) {
  TransactionalFileSystem fs(mPopulatedDir, true);
  EXPECT_THROW(fs.write("../new", "new"), Exception);
  EXPECT_THROW(fs.write("../populated/new", "new"), Exception);
  EXPECT_FALSE(mPopulatedDir.getParentDir().getPathTo("new").isExistingFile());
  EXPECT_FALSE(fs.fileExists("new"));
}

TEST_F(TransactionalFileSystemTest, testRenameFileBreakout) {
  TransactionalFileSystem fs(mPopulatedDir, true);
  EXPECT_THROW(fs.renameFile("../1.txt", "new"), Exception);
  EXPECT_THROW(fs.renameFile("1.txt", "../new"), Exception);
  EXPECT_FALSE(fs.fileExists("new"));
  EXPECT_FALSE(mPopulatedDir.getParentDir().getPathTo("new").isExistingFile());
}

TEST_F(TransactionalFileSystemTest, testRemoveFileBreakout) {
  TransactionalFileSystem fs(mPopulatedDir, true);
  EXPECT_THROW(fs.removeFile("../1.txt"), Exception);
  EXPECT_TRUE(mPopulatedDir.getParentDir().getPathTo("1.txt").isExistingFile());
}

TEST_F(TransactionalFileSystemTest, testRemoveDirRecursivelyBreakout) {
  TransactionalFileSystem fs(mPopulatedDir, true);
  EXPECT_THROW(fs.removeDirRecursively("../"), Exception);
  EXPECT_TRUE(mPopulatedDir.getParentDir().getPathTo("1.txt").isExistingFile());
}

/*******************************************************************************
 *  Parametrized getSubDirs() Tests
 ******************************************************************************/

struct TransactionalFileSystemGetSubDirsTestData {
  QString root;
  QString relPath;
  QStringList entries;
};

class TransactionalFileSystemGetSubDirsTest
  : public TransactionalFileSystemTest,
    public ::testing::WithParamInterface<
        TransactionalFileSystemGetSubDirsTestData> {};

TEST_P(TransactionalFileSystemGetSubDirsTest, testGetSubDirs) {
  const TransactionalFileSystemGetSubDirsTestData& data = GetParam();

  TransactionalFileSystem fs(mTmpDir.getPathTo(data.root), false);
  EXPECT_EQ(data.entries.count(), fs.getDirs(data.relPath).count());
  EXPECT_EQ(Toolbox::toSet(data.entries),
            Toolbox::toSet(fs.getDirs(data.relPath)));
}

// clang-format off
static TransactionalFileSystemGetSubDirsTestData sGetSubDirsTestData[] = {
// root,          relPath,            entries
  {"nonexisting", "",                 {}},
  {"nonexisting", "foo",              {}},
  {"nonexisting", "foo/bar",          {}},
  {"empty",       "",                 {}},
  {"empty",       "foo",              {}},
  {"empty",       "foo/bar",          {}},
  {"populated",   "",                 {".dot", "1", "a", "foo dir"}},
  {"populated",   ".dot",             {"dir"}},
  {"populated",   ".dot/dir",         {}},
  {"populated",   "1",                {"2"}},
  {"populated",   "1/2",              {"3"}},
  {"populated",   "1/2/3",            {}},
  {"populated",   "1/2/3/4",          {}},
  {"populated",   "a",                {"b"}},
  {"populated",   "a/b",              {}},
  {"populated",   "foo dir",          {"bar dir"}},
  {"populated",   "foo dir/bar dir",  {}},
  {"populated",   "2",                {}},
  {"populated",   "3",                {}},
  {"populated",   "b",                {}},
  {"populated",   "c",                {}},
  {"populated",   "bar dir",          {}},
  {"populated",   "hello",            {}},
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(TransactionalFileSystemGetSubDirsTest,
                         TransactionalFileSystemGetSubDirsTest,
                         ::testing::ValuesIn(sGetSubDirsTestData));

/*******************************************************************************
 *  Parametrized getFilesInDir() Tests
 ******************************************************************************/

struct TransactionalFileSystemGetFilesInDirTestData {
  QString root;
  QString relPath;
  QStringList entries;
};

class TransactionalFileSystemGetFilesInDirTest
  : public TransactionalFileSystemTest,
    public ::testing::WithParamInterface<
        TransactionalFileSystemGetFilesInDirTestData> {};

TEST_P(TransactionalFileSystemGetFilesInDirTest, testGetFilesInDir) {
  const TransactionalFileSystemGetFilesInDirTestData& data = GetParam();

  TransactionalFileSystem fs(mTmpDir.getPathTo(data.root), false);
  EXPECT_EQ(data.entries.count(), fs.getFiles(data.relPath).count());
  EXPECT_EQ(Toolbox::toSet(data.entries),
            Toolbox::toSet(fs.getFiles(data.relPath)));
}

// clang-format off
static TransactionalFileSystemGetFilesInDirTestData sGetFilesInDirTestData[] = {
// root,          relPath,            entries
  {"nonexisting", "",                 {}},
  {"nonexisting", "foo",              {}},
  {"nonexisting", "foo/bar",          {}},
  {"empty",       "",                 {}},
  {"empty",       "foo",              {}},
  {"empty",       "foo/bar",          {}},
  {"populated",   "",                 {"1.txt", "2.txt"}},
  {"populated",   ".dot",             {"file.txt"}},
  {"populated",   ".dot/dir",         {"foo.txt"}},
  {"populated",   "1",                {"1a.txt", "1b.txt"}},
  {"populated",   "1/2",              {}},
  {"populated",   "1/2/3",            {"4.txt"}},
  {"populated",   "1/2/3/4",          {}},
  {"populated",   "a",                {}},
  {"populated",   "a/b",              {"c"}},
  {"populated",   "foo dir",          {"bar dir.txt"}},
  {"populated",   "foo dir/bar dir",  {"X"}},
  {"populated",   "2",                {}},
  {"populated",   "3",                {}},
  {"populated",   "b",                {}},
  {"populated",   "c",                {}},
  {"populated",   "bar dir",          {}},
  {"populated",   "hello",            {}},
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(TransactionalFileSystemGetFilesInDirTest,
                         TransactionalFileSystemGetFilesInDirTest,
                         ::testing::ValuesIn(sGetFilesInDirTestData));

/*******************************************************************************
 *  Parametrized fileExists(), read() and readIfExists() Tests
 ******************************************************************************/

struct TransactionalFileSystemFileExistsTestData {
  QString root;
  QString relPath;
  QByteArray content;  // null = non-existing file
};

class TransactionalFileSystemFileExistsTest
  : public TransactionalFileSystemTest,
    public ::testing::WithParamInterface<
        TransactionalFileSystemFileExistsTestData> {};

TEST_P(TransactionalFileSystemFileExistsTest, testFileExists) {
  const TransactionalFileSystemFileExistsTestData& data = GetParam();

  TransactionalFileSystem fs(mTmpDir.getPathTo(data.root), false);
  EXPECT_EQ(!data.content.isNull(), fs.fileExists(data.relPath));
}

TEST_P(TransactionalFileSystemFileExistsTest, testRead) {
  const TransactionalFileSystemFileExistsTestData& data = GetParam();

  TransactionalFileSystem fs(mTmpDir.getPathTo(data.root), false);
  if (data.content.isNull()) {
    EXPECT_THROW(fs.read(data.relPath), Exception);
  } else {
    EXPECT_EQ(data.content, fs.read(data.relPath));
  }
}

TEST_P(TransactionalFileSystemFileExistsTest, testReadIfExists) {
  const TransactionalFileSystemFileExistsTestData& data = GetParam();

  TransactionalFileSystem fs(mTmpDir.getPathTo(data.root), false);
  EXPECT_EQ(data.content, fs.readIfExists(data.relPath));
  EXPECT_EQ(data.content.isNull(), fs.readIfExists(data.relPath).isNull());
}

// clang-format off
static TransactionalFileSystemFileExistsTestData sFileExistsTestData[] = {
// root,          relPath,                content
  {"nonexisting", "",                     QByteArray()},
  {"nonexisting", "foo",                  QByteArray()},
  {"empty",       "",                     QByteArray()},
  {"empty",       "foo/bar",              QByteArray()},
  {"populated",   "",                     QByteArray()},
  {"populated",   "1.txt",                "1"},
  {"populated",   "2.txt",                "2"},
  {"populated",   ".dot/file.txt",        "file"},
  {"populated",   ".dot/dir/foo.txt",     "foo"},
  {"populated",   "1",                    QByteArray()},
  {"populated",   "1/1a.txt",             "1a"},
  {"populated",   "1/1b.txt",             "1b"},
  {"populated",   "1/2",                  QByteArray()},
  {"populated",   "1/2/3/4.txt",          "4"},
  {"populated",   "1/2/3/4",              QByteArray()},
  {"populated",   "a",                    QByteArray()},
  {"populated",   "a/b/c",                "c"},
  {"populated",   "foo dir/bar dir.txt",  "bar"},
  {"populated",   "foo dir/bar dir/X",    "X"},
  {"populated",   "2",                    QByteArray()},
  {"populated",   "hello",                QByteArray()},
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(TransactionalFileSystemFileExistsTest,
                         TransactionalFileSystemFileExistsTest,
                         ::testing::ValuesIn(sFileExistsTestData));

/*******************************************************************************
 *  Parametrized cleanPath() Tests
 ******************************************************************************/

struct TransactionalFileSystemCleanPathTestData {
  QString input;
  QString output;
};

class TransactionalFileSystemCleanPathTest
  : public TransactionalFileSystemTest,
    public ::testing::WithParamInterface<
        TransactionalFileSystemCleanPathTestData> {};

TEST_P(TransactionalFileSystemCleanPathTest, testCleanPath) {
  const TransactionalFileSystemCleanPathTestData& data = GetParam();

  EXPECT_EQ(data.output, TransactionalFileSystem::cleanPath(data.input));
}

// clang-format off
static TransactionalFileSystemCleanPathTestData sCleanPathTestData[] = {
// input,                             output
  {"",                                ""},
  {"   ",                             ""},
  {".",                               ""},
  {"..",                              ".."},
  {"../",                             ".."},
  {"foo bar",                         "foo bar"},
  {"/foo\\\\bar/",                    "foo/bar"},
  {" /hello world/foo bar/.txt ",     "hello world/foo bar/.txt"},
  {"///HELLO/\\\\/FOO///",            "HELLO/FOO"},
  {"  /\\  Hello World  \\/  ",       "Hello World"},
  {"foo/../bar",                      "bar"},
  {"foo/bar/../././.",                "foo"},
  {"./foo/bar/hello/../..",           "foo"},
  {"./foo/bar/hello/../../",          "foo"},
};
// clang-format on

INSTANTIATE_TEST_SUITE_P(TransactionalFileSystemCleanPathTest,
                         TransactionalFileSystemCleanPathTest,
                         ::testing::ValuesIn(sCleanPathTestData));

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
