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
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectloader.h>
#include <librepcb/editor/project/cmd/cmdcombineboardnetsegments.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class CmdCombineBoardNetSegmentsTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(CmdCombineBoardNetSegmentsTest, testUuidsAreKept) {
  FilePath projectFp(TEST_DATA_DIR "/projects/Gerber Test/project.lpp");
  std::shared_ptr<TransactionalFileSystem> projectFs =
      TransactionalFileSystem::openRO(projectFp.getParentDir());
  ProjectLoader loader;
  std::unique_ptr<Project> project =
      loader.open(std::make_unique<TransactionalDirectory>(projectFs),
                  projectFp.getFilename());  // can throw
  Board& board = *project->getBoards().first();

  // Combine a segment with vias and junctions into another one of its net.
  BI_NetSegment* removed = nullptr;
  BI_NetSegment* result = nullptr;
  for (BI_NetSegment* a : board.getNetSegments()) {
    for (BI_NetSegment* b : board.getNetSegments()) {
      if ((a != b) && a->getNetSignal() &&
          (a->getNetSignal() == b->getNetSignal()) &&
          (!a->getVias().isEmpty()) && (!a->getNetPoints().isEmpty()) &&
          (!b->getVias().isEmpty())) {
        removed = a;
        result = b;
      }
    }
  }
  ASSERT_TRUE(removed && result);
  BI_NetPoint* oldAnchor = removed->getNetPoints().first();
  const QList<Uuid> vias = removed->getVias().keys();
  QList<Uuid> netPoints = removed->getNetPoints().keys();
  netPoints.removeOne(oldAnchor->getUuid());
  const QList<Uuid> netLines = removed->getNetLines().keys();
  ASSERT_FALSE(netPoints.isEmpty());
  ASSERT_FALSE(netLines.isEmpty());
  CmdCombineBoardNetSegments cmd(*removed, *oldAnchor, *result,
                                 *result->getVias().first());
  ASSERT_TRUE(cmd.execute());

  // Verify that the UUIDs have not changed.
  for (const Uuid& uuid : vias) {
    EXPECT_TRUE(result->getVias().contains(uuid)) << uuid.toStr().toStdString();
  }
  for (const Uuid& uuid : std::as_const(netPoints)) {
    EXPECT_TRUE(result->getNetPoints().contains(uuid))
        << uuid.toStr().toStdString();
  }
  for (const Uuid& uuid : netLines) {
    EXPECT_TRUE(result->getNetLines().contains(uuid))
        << uuid.toStr().toStdString();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
