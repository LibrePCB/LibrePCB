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
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardfabricationoutputsettings.h>
#include <librepcb/core/project/board/boardgerberexport.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectloader.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

/**
 * @brief The BoardGerberExportTest checks if boards are correctly exported to
 *        Gerber files
 *
 * The test data directory contains a "Gerber Test" project and a directory
 * containing the expected Gerber output. This test exports the Gerber files
 * of the board into a directory "actual" and compares them with the Gerber
 * files in the directory "expected". If the files differ, the test fails.
 *
 * Thus this test will fail after every change affecting the Gerber export. In
 * this case, just copy the files from "actual" to "expected", check the diff
 * with Git (i.e. verify if the diff is as expected and makes sense) and then
 * commit those changes.
 */
class BoardGerberExportTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST(BoardGerberExportTest, test) {
  FilePath testDataDir(TEST_DATA_DIR
                       "/unittests/librepcbproject/BoardGerberExportTest");

  // open project from test data directory
  FilePath projectFp(TEST_DATA_DIR "/projects/Gerber Test/project.lpp");
  std::shared_ptr<TransactionalFileSystem> projectFs =
      TransactionalFileSystem::openRO(projectFp.getParentDir());
  ProjectLoader loader;
  std::unique_ptr<Project> project =
      loader.open(std::unique_ptr<TransactionalDirectory>(
                      new TransactionalDirectory(projectFs)),
                  projectFp.getFilename());

  // force planes rebuild
  Board* board = project->getBoards().first();
  board->rebuildAllPlanes();

  // export fabrication data
  BoardFabricationOutputSettings config = board->getFabricationOutputSettings();
  config.setOutputBasePath(testDataDir.getPathTo("actual").toStr() %
                           "/{{PROJECT}}");
  BoardGerberExport grbExport(*board);
  grbExport.exportPcbLayers(config);
  grbExport.exportComponentLayer(
      BoardGerberExport::BoardSide::Top,
      testDataDir.getPathTo("actual/test_project_ASSEMBLY-TOP.gbr"));
  grbExport.exportComponentLayer(
      BoardGerberExport::BoardSide::Bottom,
      testDataDir.getPathTo("actual/test_project_ASSEMBLY-BOTTOM.gbr"));

  // replace volatile data in exported files with well-known, constant data
  foreach (const FilePath& fp, grbExport.getWrittenFiles()) {
    QString content = FileUtils::readFile(fp);
    content.replace(QRegularExpression(
                        "TF\\.GenerationSoftware,LibrePCB,LibrePCB,[^\\s\\*]*"),
                    "TF.GenerationSoftware,LibrePCB,LibrePCB,0.1.2");
    content.replace(QRegularExpression("TF\\.CreationDate,[^\\s\\*]*"),
                    "TF.CreationDate,2019-01-02T03:04:05");
    content.replace(QRegularExpression(".*TF\\.MD5,.*"), "");
    FileUtils::writeFile(fp, content.toUtf8());
  }

  // On Windows, abort here and skip this test because on AppVeyor the generated
  // Gerber files are slightly different. See discussion here:
  // https://github.com/LibrePCB/LibrePCB/pull/511#issuecomment-529089212
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
  GTEST_SKIP();
#endif

  // compare generated files with expected content
  foreach (const FilePath& fp, grbExport.getWrittenFiles()) {
    QString actual = FileUtils::readFile(fp);
    QString expected = FileUtils::readFile(
        testDataDir.getPathTo("expected").getPathTo(fp.getFilename()));
    EXPECT_EQ(expected.toStdString(), actual.toStdString());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
