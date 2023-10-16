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

#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)  // Windows
#include <fileapi.h>
#endif

#include <QtCore>

#include <fstream>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

static void setupFile(const FilePath& pth, const QByteArray& content,
                      bool hidden = false) {
  auto filename = pth.toNative().toStdString();
  std::fstream fs{filename, std::ios::out | std::ios::trunc | std::ios::binary};
  EXPECT_TRUE(fs.is_open());
  fs.write(content.data(), content.size());
  fs.close();

  if (hidden) {
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
    SetFileAttributes(pth.toNative().toStdWString().c_str(),
                      FILE_ATTRIBUTE_HIDDEN);
#endif
  }
}

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class FileUtilsTest : public ::testing::Test {
protected:
  FilePath root{FilePath::getRandomTempPath()};

  // the sources of already existing files and directories
  FilePath rootFile{root.getPathTo("file.txt")};  // source for all operations
  FilePath rootFileHidden{root.getPathTo(".hidden.txt")};

  FilePath subdir{root.getPathTo("subdir")};
  FilePath subdirFile{subdir.getPathTo("file.txt")};
  FilePath subdirSubdir{subdir.getPathTo("subdir")};
  FilePath subdirSubdirFile{subdirSubdir.getPathTo("file.txt")};
  FilePath subdirSubdirFileHidden{subdirSubdir.getPathTo(".hidden.txt")};

  // the destinations for copying files, nonexistent at start of test
  FilePath rootFileMissing{root.getPathTo("missing.txt")};
  FilePath rootFileCopy{root.getPathTo("fileCopy.txt")};
  FilePath subdirCopy{root.getPathTo("subdirCopy")};
  FilePath subdirCopyFile{subdirCopy.getPathTo("file.txt")};
  FilePath subdirCopySubdir{subdirCopy.getPathTo("subdir")};
  FilePath subdirCopySubdirFile{subdirCopySubdir.getPathTo("file.txt")};
  FilePath subdirCopySubdirFileHidden{
      subdirCopySubdir.getPathTo(".hidden.txt")};

  QStringList filter{"*.txt"};

  FileUtilsTest() {
    QDir().mkdir(root.toNative());
    QDir().mkdir(subdir.toNative());
    QDir().mkdir(subdirSubdir.toNative());

    setupFile(rootFile, "test\n");
    setupFile(rootFileHidden, "hiddenContent\n", true);
    setupFile(subdirFile, "test\n");
    setupFile(subdirSubdirFile, "test\n");
    setupFile(subdirSubdirFileHidden, "hiddenContent\n", true);
  }

  ~FileUtilsTest() {
    // each test should clean itself after run
    QDir(root.toNative()).removeRecursively();
  }

  /**
   * @brief Make the unordered result list comparable with better test message
   *
   *  **using std::string:**
   *  Expected equality of these values:
   *    comparable(actual)
   *      Which is: "\n/tmp/librepcb/1696569432486_2075782095/file.txt..."
   *    comparable(expected)
   *      Which is: "\n/tmp/librepcb/1696569432486_2075782095/file.txt..."
   *  With diff:
   *   @@ +1,4 @@
   *
   *   +/tmp/librepcb/1696569432486_2075782095/file.txt
   *    /tmp/librepcb/1696569432486_2075782095/file.txt
   *    /tmp/librepcb/1696569432486_2075782095/subdir/file.txt
   *
   *  **using Qt's QString**:
   *  Expected equality of these values:
   *    comparable(actual)
   *      Which is: { 2-byte object <0A-00>, 2-byte object <2F-00>, ... }
   *    comparable(expected)
   *      Which is: { 2-byte object <0A-00>, 2-byte object <2F-00>, ... }
   *
   *  @param path result from getFilesInDirectory
   *  @return comparable string for EXPECT macros
   */
  static std::string comparable(const QList<FilePath>& path) {
    auto join = [](std::string a, std::string b) {
      return !a.empty() ? (std::move(a) + '\n' + std::move(b)) : std::move(b);
    };
    auto conv = [](const FilePath& filepath) {
      return filepath.toStr().toStdString();
    };
    std::vector<std::string> paths{};
    std::transform(path.begin(), path.end(), std::back_inserter(paths), conv);
    std::sort(paths.begin(), paths.end());
    return std::accumulate(paths.begin(), paths.end(), std::string(), join);
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(FileUtilsTest, testReadExistingFile) {
  auto p = FileUtils::readFile(rootFile);

  EXPECT_EQ("test\n", p.toStdString());
}

TEST_F(FileUtilsTest, testReadNonexistentFileShouldThrow) {
  EXPECT_THROW(FileUtils::readFile(rootFileMissing), Exception);
}

TEST_F(FileUtilsTest, testWrittenDataShouldBeReadBack) {
  FileUtils::writeFile(rootFile, "someData\n");
  auto p = FileUtils::readFile(rootFile);

  EXPECT_EQ("someData\n", p.toStdString());
}

TEST_F(FileUtilsTest, testCopyValidFile) {
  FileUtils::copyFile(rootFile, rootFileCopy);
  auto p1 = FileUtils::readFile(rootFile);
  auto p2 = FileUtils::readFile(rootFileCopy);  // throws if file not found

  EXPECT_EQ("test\n", p1.toStdString());
  EXPECT_EQ("test\n", p2.toStdString());
}

TEST_F(FileUtilsTest, testCopyNonexistingFileShouldThrow) {
  EXPECT_THROW(FileUtils::copyFile(rootFileMissing, rootFileCopy), Exception);
}

TEST_F(FileUtilsTest, testMoveValidFile) {
  FileUtils::move(rootFile, rootFileCopy);
  auto p = FileUtils::readFile(rootFileCopy);  // throws if file not found

  EXPECT_THROW(FileUtils::readFile(rootFile), Exception);
  EXPECT_EQ("test\n", p.toStdString());
}

TEST_F(FileUtilsTest, testMoveNonexistingFileShouldThrow) {
  EXPECT_THROW(FileUtils::move(rootFileMissing, rootFileCopy), Exception);
}

TEST_F(FileUtilsTest, testRemoveValidFile) {
  FileUtils::removeFile(rootFile);

  EXPECT_THROW(FileUtils::readFile(rootFile), Exception);
}

TEST_F(FileUtilsTest, testRemoveNonexistingFileShouldThrow) {
  EXPECT_THROW(FileUtils::removeFile(rootFileMissing), Exception);
}

TEST_F(FileUtilsTest, testCreateSubdir) {
  FileUtils::makePath(subdirCopy);

  EXPECT_TRUE(subdirCopy.isExistingDir());
}

TEST_F(FileUtilsTest, testRecursiveRemoveSubdir) {
  FileUtils::removeDirRecursively(subdir);

  EXPECT_FALSE(subdir.isExistingDir());
  EXPECT_FALSE(subdirFile.isExistingFile());
}

TEST_F(FileUtilsTest, testRecursiveCopySubdir) {
  FileUtils::copyDirRecursively(subdir, subdirCopy);

  // ensure source remain unchanged
  EXPECT_TRUE(subdir.isExistingDir());
  EXPECT_TRUE(subdirFile.isExistingFile());
  EXPECT_TRUE(subdirSubdirFile.isExistingFile());
  EXPECT_TRUE(subdirSubdirFileHidden.isExistingFile());

  // ensure destination is complete copy
  EXPECT_TRUE(subdirCopy.isExistingDir());
  EXPECT_TRUE(subdirCopyFile.isExistingFile());
  EXPECT_TRUE(subdirCopySubdirFile.isExistingFile());
  EXPECT_TRUE(subdirCopySubdirFileHidden.isExistingFile());
}

TEST_F(FileUtilsTest, testFindDirectories) {
  auto actual = FileUtils::findDirectories(root);
  auto expected = QList<FilePath>{subdir};

  EXPECT_EQ(comparable(actual), comparable(expected));
}

TEST_F(FileUtilsTest, testGetFilesInDirectory) {
  auto actual = FileUtils::getFilesInDirectory(root);
  auto expected = QList<FilePath>{rootFile, rootFileHidden};

  // Those should be skipped:
  // * subdirFile, (not recursive)
  // * subdirSubdirFile, (not recursive)
  // * subdirSubdirFileHidden (not recursive)

  EXPECT_EQ(comparable(actual), comparable(expected));
}

TEST_F(FileUtilsTest, testGetFilesInDirectoryRecursive) {
  auto actual = FileUtils::getFilesInDirectory(root, {}, true);
  auto expected = QList<FilePath>{
      rootFile,         rootFileHidden,         subdirFile,
      subdirSubdirFile, subdirSubdirFileHidden,
  };

  EXPECT_EQ(comparable(actual), comparable(expected));
}

TEST_F(FileUtilsTest, testGetFilesInDirectorySkipHidden) {
  auto actual = FileUtils::getFilesInDirectory(root, {}, false, true);
  auto expected = QList<FilePath>{rootFile};

  // Those should be skipped in output:
  // * rootFileHidden  (hidden)
  // * subdirFile  (not recursive)
  // * subdirSubdirFile (not recursive)
  // * subdirSubdirFileHidden (not recursive, hidden)

  EXPECT_EQ(comparable(actual), comparable(expected));
}

TEST_F(FileUtilsTest, testGetFilesInDirectoryRecursiveSkipHidden) {
  auto actual = FileUtils::getFilesInDirectory(root, {}, true, true);
  auto expected = QList<FilePath>{rootFile, subdirFile, subdirSubdirFile};

  // Those should be skipped in output:
  // * rootFileHidden (hidden)
  // * subdirSubdirFileHidden (hidden)

  EXPECT_EQ(comparable(actual), comparable(expected));
}

TEST_F(FileUtilsTest, testGetFilesInDirectoryFiltered) {
  auto actual = FileUtils::getFilesInDirectory(root, filter, false);
  auto expected = QList<FilePath>{rootFile, rootFileHidden};

  // Those should be skipped in output:
  // * subdirFile (not recursive)
  // * subdirSubdirFile (not recursive)
  // * subdirSubdirFileHidden (not recursive)

  EXPECT_EQ(comparable(actual), comparable(expected));
}

TEST_F(FileUtilsTest, testGetFilesInDirectoryRecursiveFiltered) {
  auto actual = FileUtils::getFilesInDirectory(root, filter, true);
  auto expected = QList<FilePath>{rootFile, rootFileHidden, subdirFile,
                                  subdirSubdirFile, subdirSubdirFileHidden};

  EXPECT_EQ(comparable(actual), comparable(expected));
}

TEST_F(FileUtilsTest, testGetFilesInDirectoryRecursiveFilteredSkipHidden) {
  auto actual = FileUtils::getFilesInDirectory(root, filter, true, true);
  auto expected = QList<FilePath>{rootFile, subdirFile, subdirSubdirFile};

  // Those should be skipped in output:
  // * rootFileHidden skipped (hidden)
  // * subdirSubdirFileHidden skipped (hidden)

  EXPECT_EQ(comparable(actual), comparable(expected));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
