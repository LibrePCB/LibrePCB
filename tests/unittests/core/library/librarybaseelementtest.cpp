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
#include <librepcb/library/librarybaseelement.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class LibraryBaseElementTest : public ::testing::Test {
protected:
  FilePath mTempDir;
  QScopedPointer<LibraryBaseElement> mNewElement;

  LibraryBaseElementTest() {
    mTempDir = FilePath::getRandomTempPath();

    mNewElement.reset(new LibraryBaseElement(
        true, "sym", "symbol", Uuid::createRandom(), Version::fromString("1.0"),
        "test", ElementName("Test"), "", ""));
  }

  virtual ~LibraryBaseElementTest() {
    QDir(mTempDir.toStr()).removeRecursively();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(LibraryBaseElementTest, testSave) {
  mNewElement->save();
}

TEST_F(LibraryBaseElementTest, testMoveToNonExistingDirectory) {
  FilePath dest = mTempDir.getPathTo(mNewElement->getUuid().toStr());
  std::shared_ptr<TransactionalFileSystem> destFileSystem =
      TransactionalFileSystem::openRW(dest);
  TransactionalDirectory destDir(destFileSystem);
  mNewElement->moveTo(destDir);
  EXPECT_TRUE(destFileSystem->fileExists("symbol.lp"));
  destFileSystem->save();
  EXPECT_TRUE(dest.getPathTo("symbol.lp").isExistingFile());
}

TEST_F(LibraryBaseElementTest, testMoveToEmptyDirectory) {
  // Saving into empty destination directory must work because empty directories
  // are sometimes created "accidentally" (for example by Git operations which
  // remove files, but not their parent directories). So we handle empty
  // directories like they are not existent...
  FilePath dest = mTempDir.getPathTo(mNewElement->getUuid().toStr());
  FileUtils::makePath(dest);
  ASSERT_TRUE(dest.isExistingDir());
  std::shared_ptr<TransactionalFileSystem> destFileSystem =
      TransactionalFileSystem::openRW(dest);
  TransactionalDirectory destDir(destFileSystem);
  mNewElement->moveTo(destDir);
  EXPECT_TRUE(destFileSystem->fileExists("symbol.lp"));
  destFileSystem->save();
  EXPECT_TRUE(dest.getPathTo("symbol.lp").isExistingFile());
}

// Currently disabled because of the file system refactoring, and not sure if
// this behavior is really what we want...
//
// TEST_F(LibraryBaseElementTest, testMoveToNonEmptyDirectory) {
//  // Saving into non-empty destination directory must fail because we may
//  // accidentally overwrite existing files!
//  FilePath dest = mTempDir.getPathTo(mNewElement->getUuid().toStr());
//  FileUtils::writeFile(dest.getPathTo("some file"), "some content");
//  EXPECT_THROW(mNewElement->saveTo(dest), RuntimeError);
//}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace library
}  // namespace librepcb
