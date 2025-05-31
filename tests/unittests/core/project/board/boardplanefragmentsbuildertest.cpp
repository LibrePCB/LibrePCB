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
#include <librepcb/core/types/layer.h>

#include <QtCore>

#include <chrono>

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
  const QHash<Uuid, QVector<Path>> result =
      builder.runAndApply(*board);  // can throw

  // Check if fragments have been applied.
  foreach (const BI_Plane* plane, board->getPlanes()) {
    EXPECT_TRUE(plane->getFragments() == result[plane->getUuid()]);
  }

  // write actual plane fragments into file (useful for debugging purposes)
  std::unique_ptr<SExpression> actualSexpr = SExpression::createList("actual");
  foreach (const Uuid& uuid, Toolbox::sorted(result.keys())) {
    std::unique_ptr<SExpression> child = SExpression::createList("plane");
    child->appendChild(uuid);
    foreach (const Path& fragment, result[uuid]) {
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

  // On Apple Silicon, abort here and skip this test because on CI the
  // generated files are slightly different
  // (https://github.com/LibrePCB/LibrePCB/issues/516).
#if defined(__APPLE__) && defined(__arm64__)
  GTEST_SKIP();
#endif

  // compare with expected plane fragments loaded from file
  FilePath expectedFp = testDataDir.getPathTo("expected.lp");
  QByteArray expected = FileUtils::readFile(expectedFp);
  EXPECT_EQ(expected.toStdString(), actual.toStdString());
}

TEST(BoardPlaneFragmentsBuilderTest, testManyThreads) {
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

  // Copy planes on top layer to more layers, otherwise this test is quite
  // meaningless.
  board->setInnerLayerCount(40);
  const QSet<const Layer*> otherLayers =
      board->getCopperLayers() - QSet<const Layer*>{&Layer::topCopper()};
  foreach (BI_Plane* plane, board->getPlanes()) {
    foreach (const Layer* layer, otherLayers) {
      BI_Plane* newPlane =
          new BI_Plane(*board, Uuid::createRandom(), *layer,
                       plane->getNetSignal(), plane->getOutline());
      board->addPlane(*newPlane);
    }
  }
  std::cout << "Testing with " << board->getPlanes().count() << " planes on "
            << board->getCopperLayers().count() << " layers.\n";

  // Run several times to heavily test multithreading.
  // Note: On macOS CI we sometimes get a segfault which is not reproducible on
  // other platforms. It's not clear yet where this comes from, but it was
  // already there before introducing true parallelization with threads. For now
  // we make the test way simpler on macOS to avoid frequent CI failures.
  const int runs =
#if defined(Q_OS_MACOS)
      1;
#else
      qBound(10, QThread::idealThreadCount() * 8, 50);
#endif
  qreal totalTimeMs = 0;
  BoardPlaneFragmentsBuilder builder;
  QHash<Uuid, QVector<Path>> firstResult;
  for (int i = 0; i < runs; ++i) {
    std::chrono::time_point<std::chrono::high_resolution_clock> start =
        std::chrono::high_resolution_clock::now();
    builder.start(*board);
    BoardPlaneFragmentsBuilder::Result result = builder.waitForFinished();
    std::chrono::duration<double> elapsed =
        std::chrono::high_resolution_clock::now() - start;
    totalTimeMs += elapsed.count() * 1000;

    // Check result.
    EXPECT_EQ(board, result.board);
    EXPECT_EQ(0, result.errors.count());
    EXPECT_TRUE(result.finished);

    // Check if every run leads to the same plane fragments.
    if (i == 0) {
      firstResult = result.planes;
    } else {
      EXPECT_TRUE(result.planes == firstResult);
    }
  }
  std::cout << "Average time over " << runs << " runs: " << (totalTimeMs / runs)
            << " ms\n";
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
