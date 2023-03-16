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
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/project/board/boardd356netlistexport.h>
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

class BoardD356NetlistExportTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(BoardD356NetlistExportTest, test) {
  FilePath testDataDir(TEST_DATA_DIR
                       "/unittests/librepcbproject/BoardD356NetlistExportTest");

  // Open project from test data directory.
  FilePath projectFp(TEST_DATA_DIR "/projects/Gerber Test/project.lpp");
  std::shared_ptr<TransactionalFileSystem> projectFs =
      TransactionalFileSystem::openRO(projectFp.getParentDir());
  ProjectLoader loader;
  std::unique_ptr<Project> project =
      loader.open(std::unique_ptr<TransactionalDirectory>(
                      new TransactionalDirectory(projectFs)),
                  projectFp.getFilename());  // can throw

  // Export netlist.
  const FilePath fp = testDataDir.getPathTo("actual/netlist.d356");
  Board* board = project->getBoards().first();
  BoardD356NetlistExport exp(*board);
  QString content = exp.generate();  // can throw

  // Replace volatile data in exported files with well-known, constant data.
  content.replace(QRegularExpression("Generation Software: LibrePCB (.*)"),
                  "Generation Software:");
  content.replace(QRegularExpression("Generation Date: (.*)"),
                  "Generation Date:");
  FileUtils::writeFile(fp, content.toUtf8());

  // Compare generated file with expected content.
  const QString expected = FileUtils::readFile(
      testDataDir.getPathTo("expected").getPathTo(fp.getFilename()));
  EXPECT_EQ(expected.toStdString(), content.toStdString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
