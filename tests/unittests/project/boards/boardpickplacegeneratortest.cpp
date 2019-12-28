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
#include <librepcb/common/fileio/csvfile.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/fileio/transactionaldirectory.h>
#include <librepcb/common/fileio/transactionalfilesystem.h>
#include <librepcb/common/pnp/pickplacecsvwriter.h>
#include <librepcb/common/pnp/pickplacedata.h>
#include <librepcb/project/boards/boardpickplacegenerator.h>
#include <librepcb/project/project.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class BoardPickPlaceGeneratorTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(BoardPickPlaceGeneratorTest, test) {
  FilePath testDataDir(
      TEST_DATA_DIR "/unittests/librepcbproject/BoardPickPlaceGeneratorTest");

  // open project from test data directory
  FilePath projectFp(TEST_DATA_DIR "/projects/Gerber Test/project.lpp");
  std::shared_ptr<TransactionalFileSystem> projectFs =
      TransactionalFileSystem::openRO(projectFp.getParentDir());
  QScopedPointer<Project> project(
      new Project(std::unique_ptr<TransactionalDirectory>(
                      new TransactionalDirectory(projectFs)),
                  projectFp.getFilename()));

  // export pick&place data
  QList<FilePath>                writtenFiles;
  Board*                         board = project->getBoards().first();
  BoardPickPlaceGenerator        gen(*board);
  std::shared_ptr<PickPlaceData> data = gen.generate();
  PickPlaceCsvWriter             writer(*data);

  // top devices with comment
  {
    writer.setBoardSide(PickPlaceCsvWriter::BoardSide::TOP);
    writer.setIncludeMetadataComment(true);
    FilePath fp = testDataDir.getPathTo("actual/top.csv");
    writer.generateCsv()->saveToFile(fp);
    writtenFiles.append(fp);
  }

  // bottom devices with comment
  {
    writer.setBoardSide(PickPlaceCsvWriter::BoardSide::BOTTOM);
    writer.setIncludeMetadataComment(true);
    FilePath fp = testDataDir.getPathTo("actual/bottom.csv");
    writer.generateCsv()->saveToFile(fp);
    writtenFiles.append(fp);
  }

  // top+bottom devices without comment
  {
    writer.setBoardSide(PickPlaceCsvWriter::BoardSide::BOTH);
    writer.setIncludeMetadataComment(false);
    FilePath fp = testDataDir.getPathTo("actual/both.csv");
    writer.generateCsv()->saveToFile(fp);
    writtenFiles.append(fp);
  }

  // replace volatile data in exported files with well-known, constant data
  foreach (const FilePath& fp, writtenFiles) {
    QString content = FileUtils::readFile(fp);
    content.replace(QRegularExpression("Generation Software:(.*)"),
                    "Generation Software:");
    content.replace(QRegularExpression("Generation Date:(.*)"),
                    "Generation Date:");
    FileUtils::writeFile(fp, content.toUtf8());
  }

  // compare generated files with expected content
  foreach (const FilePath& fp, writtenFiles) {
    QString actual   = FileUtils::readFile(fp);
    QString expected = FileUtils::readFile(
        testDataDir.getPathTo("expected").getPathTo(fp.getFilename()));
    EXPECT_EQ(expected.toStdString(), actual.toStdString());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace project
}  // namespace librepcb
