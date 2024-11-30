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
#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/kicadimport/kicadlibraryimport.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace kicadimport {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class KiCadLibraryImportTest : public ::testing::Test {
protected:
  FilePath mWsDir;
  std::unique_ptr<WorkspaceLibraryDb> mWsDb;

  KiCadLibraryImportTest() : mWsDir(FilePath::getRandomTempPath()) {
    FileUtils::makePath(mWsDir);
    mWsDb.reset(new WorkspaceLibraryDb(mWsDir));
  }

  virtual ~KiCadLibraryImportTest() {
    QDir(mWsDir.toStr()).removeRecursively();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(KiCadLibraryImportTest, testImport) {
  const FilePath src(TEST_DATA_DIR "/unittests/kicadimport");
  const FilePath dst = FilePath::getRandomTempPath();

  KiCadLibraryImport import(*mWsDb, dst);
  std::shared_ptr<MessageLogger> log = std::make_shared<MessageLogger>();

  // Connect signals by hand because QSignalSpy is not threadsafe!
  int signalFinished = 0;
  QObject::connect(&import, &KiCadLibraryImport::scanFinished,
                   [&signalFinished]() { ++signalFinished; });
  QObject::connect(&import, &KiCadLibraryImport::parseFinished,
                   [&signalFinished]() { ++signalFinished; });
  QObject::connect(&import, &KiCadLibraryImport::importFinished,
                   [&signalFinished]() { ++signalFinished; });

  // Scan.
  bool ret = import.startScan(src, FilePath(), log);
  ASSERT_TRUE(ret);
  std::shared_ptr<KiCadLibraryImport::Result> result = import.getResult();
  EXPECT_EQ(1, signalFinished);
  EXPECT_GE(log->getMessages().count(), 1);
  ASSERT_EQ(1, result->symbolLibs.count());
  ASSERT_EQ(0, result->symbolLibs.first().symbols.count());
  ASSERT_EQ(1, result->footprintLibs.count());
  ASSERT_EQ(1, result->footprintLibs.first().files.count());
  ASSERT_EQ(0, result->footprintLibs.first().footprints.count());
  log->clear();
  signalFinished = 0;

  // Parse.
  ret = import.startParse(log);
  ASSERT_TRUE(ret);
  result = import.getResult();
  EXPECT_EQ(1, signalFinished);
  EXPECT_GE(log->getMessages().count(), 1);
  ASSERT_EQ(1, result->symbolLibs.count());
  ASSERT_EQ(1, result->symbolLibs.first().symbols.count());
  ASSERT_EQ(1, result->footprintLibs.count());
  ASSERT_EQ(1, result->footprintLibs.first().files.count());
  ASSERT_EQ(1, result->footprintLibs.first().footprints.count());
  log->clear();
  signalFinished = 0;

  // Verify nothing is exported yet.
  EXPECT_FALSE(dst.isExistingDir());

  // Import.
  ret = import.startImport(log);
  ASSERT_TRUE(ret);
  result = import.getResult();
  EXPECT_EQ(1, signalFinished);
  EXPECT_GE(log->getMessages().count(), 1);
  log->clear();
  signalFinished = 0;

  // Verify that files have been written (2 files per element).
  const QList<FilePath> dstFiles =
      FileUtils::getFilesInDirectory(dst, QStringList(), true, false);
  EXPECT_EQ(6, dstFiles.count());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace kicadimport
}  // namespace librepcb
