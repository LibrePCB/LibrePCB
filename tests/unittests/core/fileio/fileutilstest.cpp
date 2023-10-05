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
  std::fstream file{filename, std::ios::out | std::ios::trunc};
  EXPECT_TRUE(file.is_open());
  file.write(content.data(), content.size());
  file.close();

  if (hidden) {
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
    SetFileAttributes(pth.toNative().toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
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
  FilePath subdirCopySubdirFileHidden{subdirCopySubdir.getPathTo(".hidden.txt")};

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

  static void expectList(const QList<FilePath>& result,
                         const QList<FilePath>& expected) {
    std::string resultStr;
    std::string expStr;
    for (const auto& i : result)
      resultStr += "  " + i.toStr().toStdString() + "\n";
    for (const auto& i : expected)
      expStr += "  " + i.toStr().toStdString() + "\n";

    ASSERT_EQ(result.size(), expected.size())
        << "\nActual:\n" << resultStr
        << "\nExpected:\n" << expStr;
    for (const auto& i : expected)
      ASSERT_TRUE(result.contains(i))
          << "\nActual:\n" << resultStr
          << "\nExpected:\n" << expStr;
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
  auto p = FileUtils::findDirectories(root);

  expectList(p, {subdir});
}

TEST_F(FileUtilsTest, testFindFileInDirectory) {
  auto p = FileUtils::getFilesInDirectory(root);

  expectList(p,
             {
                 rootFile, rootFileHidden,
                 // subdirFile, skipped (not recursive)
                 // subdirSubdirFile, skipped (not recursive)
                 // subdirSubdirFileHidden (not recursive)
             });
}

TEST_F(FileUtilsTest, testFindFileInDirectoryRecursive) {
  auto p = FileUtils::getFilesInDirectory(root, {}, true);

  expectList(p,
             {
                 rootFile,
                 rootFileHidden,
                 subdirFile,
                 subdirSubdirFile,
                 subdirSubdirFileHidden,
             });
}

TEST_F(FileUtilsTest, testFindFileInDirectorySkipHidden) {
  auto p = FileUtils::getFilesInDirectory(root, {}, false, true);

  expectList(p,
             {
                 rootFile,
                 // rootFileHidden, skipped (hidden)
                 // subdirFile, skipped (not recursive)
                 // subdirSubdirFile, skipped (not recursive)
                 // subdirSubdirFileHidden (not recursive, hidden)
             });
}

TEST_F(FileUtilsTest, testFindFileInDirectoryRecursiveSkipHidden) {
  auto p = FileUtils::getFilesInDirectory(root, {}, true, true);

  expectList(p,
             {
                 rootFile,
                 // rootFileHidden, skipped (hidden)
                 subdirFile, subdirSubdirFile,
                 // subdirSubdirFileHidden (hidden)
             });
}

TEST_F(FileUtilsTest, testFindFileInDirectoryFiltered) {
  auto p = FileUtils::getFilesInDirectory(root, filter, false);

  expectList(p,
             {
                 rootFile, rootFileHidden,
                 // subdirFile skipped (not recursive)
                 // subdirSubdirFile (not recursive)
                 // subdirSubdirFileHidden (not recursive)
             });
}

TEST_F(FileUtilsTest, testFindFileInDirectoryRecursiveFiltered) {
  auto p = FileUtils::getFilesInDirectory(root, filter, true);

  expectList(p,
             {rootFile, rootFileHidden, subdirFile, subdirSubdirFile,
              subdirSubdirFileHidden});
}

TEST_F(FileUtilsTest, testFindFileInDirectoryRecursiveFilteredSkipHidden) {
  auto p = FileUtils::getFilesInDirectory(root, filter, true, true);

  expectList(p,
             {
                 rootFile,
                 // rootFileHidden skipped (hidden)
                 subdirFile, subdirSubdirFile
                 // subdirSubdirFileHidden skipped (hidden)
             });
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
