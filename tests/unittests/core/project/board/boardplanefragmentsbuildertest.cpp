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
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardplanefragmentsbuilder.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectloader.h>
#include <librepcb/core/serialization/sexpression.h>

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
 * @brief The BoardPlaneFragmentsBuilderTest checks if board plane fragments are
 * correct
 *
 * In the test data directory is a project containing some planes and a file
 * with the expected paths of all plane fragments. This test then re-calculates
 * all plane fragments and compares them with the expected fragments.
 */
class BoardPlaneFragmentsBuilderTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST(BoardPlaneFragmentsBuilderTest, testFragments) {
  FilePath testDataDir(
      TEST_DATA_DIR
      "/unittests/librepcbproject/BoardPlaneFragmentsBuilderTest");

  // open project from test data directory
  FilePath projectFp(TEST_DATA_DIR "/projects/Nested Planes/project.lpp");
  std::shared_ptr<TransactionalFileSystem> projectFs =
      TransactionalFileSystem::openRO(projectFp.getParentDir());
  ProjectLoader loader;
  std::unique_ptr<Project> project =
      loader.open(std::unique_ptr<TransactionalDirectory>(
                      new TransactionalDirectory(projectFs)),
                  projectFp.getFilename());  // can throw
  Board* board = project->getBoards().first();

  // force planes rebuild
  BoardPlaneFragmentsBuilder builder;
  builder.runSynchronously(*board);  // can throw

  // determine actual plane fragments
  QMap<Uuid, QVector<Path>> actualPlaneFragments;
  foreach (const BI_Plane* plane, board->getPlanes()) {
    actualPlaneFragments[plane->getUuid()] = plane->getFragments();
  }

  // write actual plane fragments into file (useful for debugging purposes)
  std::unique_ptr<SExpression> actualSexpr = SExpression::createList("actual");
  foreach (const Uuid& uuid, actualPlaneFragments.keys()) {
    std::unique_ptr<SExpression> child = SExpression::createList("plane");
    child->appendChild(uuid);
    foreach (const Path& fragment, actualPlaneFragments[uuid]) {
      child->ensureLineBreak();
      fragment.serialize(child->appendList("fragment"));
    }
    child->ensureLineBreak();
    actualSexpr->ensureLineBreak();
    actualSexpr->appendChild(std::move(child));
  }
  actualSexpr->ensureLineBreak();
  QByteArray actual = actualSexpr->toByteArray();
  FileUtils::writeFile(testDataDir.getPathTo("actual.lp"), actual);

  // compare with expected plane fragments loaded from file
  FilePath expectedFp = testDataDir.getPathTo("expected.lp");
  QByteArray expected = FileUtils::readFile(expectedFp);
  EXPECT_EQ(expected.toStdString(), actual.toStdString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
