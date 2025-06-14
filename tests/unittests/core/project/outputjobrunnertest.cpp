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
#include <librepcb/core/job/gerberexcellonoutputjob.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/outputjobrunner.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/types/layer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class OutputJobRunnerTest : public ::testing::Test {
protected:
  FilePath mOutDir;

  OutputJobRunnerTest() { mOutDir = FilePath::getRandomTempPath(); }

  virtual ~OutputJobRunnerTest() {
    QDir(mOutDir.getParentDir().toStr()).removeRecursively();
  }

  std::unique_ptr<BI_Polygon> createBoardOutline(Board& board) const {
    std::unique_ptr<BI_Polygon> polygon(new BI_Polygon(
        board,
        BoardPolygonData(Uuid::createRandom(), Layer::boardOutlines(),
                         UnsignedLength(0),
                         Path::centeredRect(PositiveLength(5000000),
                                            PositiveLength(5000000)),
                         false, false, false)));
    return polygon;
  }

  std::unique_ptr<BI_Plane> createPlane(Board& board) const {
    std::unique_ptr<BI_Plane> plane(new BI_Plane(
        board, Uuid::createRandom(), *Layer::innerCopper(1), nullptr,
        Path::centeredRect(PositiveLength(5000000), PositiveLength(5000000))));
    return plane;
  }

  std::unique_ptr<Board> createBoard(Project& project) const {
    std::unique_ptr<Board> board(new Board(
        project,
        std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory()),
        "board", Uuid::createRandom(), ElementName("New Board")));
    board->setInnerLayerCount(2);
    return board;
  }

  std::unique_ptr<Project> createProject() const {
    std::unique_ptr<Project> project = Project::create(
        std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
            TransactionalFileSystem::openRW(FilePath::getRandomTempPath()))),
        "project.lpp");
    return project;
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

// Very important: Make sure the Gerber/Excellon output job rebuilds any
// outdated planes before exporting.
TEST_F(OutputJobRunnerTest, testGerberExcellonRebuildsPlanes) {
  std::unique_ptr<Project> project = createProject();
  QPointer<Board> board = createBoard(*project).release();
  board->addPolygon(*createBoardOutline(*board).release());
  QPointer<BI_Plane> plane = createPlane(*board).release();
  board->addPlane(*plane);
  project->addBoard(*board);

  std::shared_ptr<GerberExcellonOutputJob> job =
      GerberExcellonOutputJob::protelStyle();

  EXPECT_EQ(plane->getFragments().count(), 0);

  OutputJobRunner runner(*project);
  runner.setOutputDirectory(mOutDir);
  runner.run({job});

  EXPECT_GT(plane->getFragments().count(), 0);

  const FilePath fp = mOutDir.getPathTo("gerber/Unnamed_v1.g1");
  const QByteArray content = FileUtils::readFile(fp);
  EXPECT_TRUE(content.contains("\nG36*\n"));
  EXPECT_TRUE(content.contains("\nG37*\n"));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
