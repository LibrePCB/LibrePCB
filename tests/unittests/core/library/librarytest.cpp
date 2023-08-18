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
#include <librepcb/core/application.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/library.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class LibraryTest : public ::testing::Test {
protected:
  FilePath mTmpDir;

  LibraryTest() {
    // the whitespaces in the path are there to make the test even stronger ;)
    mTmpDir = FilePath::getRandomTempPath().getPathTo("test dir.lplib");
  }

  virtual ~LibraryTest() {
    QDir(mTmpDir.getParentDir().toStr()).removeRecursively();
  }

  std::unique_ptr<TransactionalDirectory> createDir(bool writable = true) const
      noexcept {
    return std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
        TransactionalFileSystem::open(mTmpDir, writable)));
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(LibraryTest, testUpgradeV01) {
  // Copy into temporary directory.
  const FilePath src(TEST_DATA_DIR "/libraries/v0.1.lplib");
  FileUtils::copyDirRecursively(src, mTmpDir);

  // Open/upgrade/close.
  ASSERT_TRUE(FileUtils::readFile(mTmpDir.getPathTo(".librepcb-lib"))
                  .startsWith("0.1\n"));
  {
    std::unique_ptr<Library> obj = Library::open(createDir());
    obj->save();
    obj->getDirectory().getFileSystem()->save();
  }

  // Re-open.
  ASSERT_TRUE(
      FileUtils::readFile(mTmpDir.getPathTo(".librepcb-lib"))
          .startsWith(Application::getFileFormatVersion().toStr().toUtf8() %
                      "\n"));
  { std::unique_ptr<Library> obj = Library::open(createDir()); }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
