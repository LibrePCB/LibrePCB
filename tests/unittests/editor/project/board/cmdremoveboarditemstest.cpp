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
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectloader.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/editor/project/cmd/cmdremoveboarditems.h>

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

class CmdRemoveBoardItemsTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(CmdRemoveBoardItemsTest, testRemainingTraceAndJunctionUuidsAreKept) {
  FilePath projectFp(TEST_DATA_DIR "/projects/Gerber Test/project.lpp");
  std::shared_ptr<TransactionalFileSystem> projectFs =
      TransactionalFileSystem::openRO(projectFp.getParentDir());
  ProjectLoader loader;
  std::unique_ptr<Project> project =
      loader.open(std::make_unique<TransactionalDirectory>(projectFs),
                  projectFp.getFilename());  // can throw
  Board& board = *project->getBoards().first();

  BI_NetSegment* segment = nullptr;
  for (BI_NetSegment* s : board.getNetSegments()) {
    if (s->getNetLines().count() >= 2) {
      segment = s;
      break;
    }
  }
  ASSERT_TRUE(segment);
  BI_NetLine* line = segment->getNetLines().first();
  const Uuid removedLine = line->getUuid();
  QSet<Uuid> keptLines = Toolbox::toSet(segment->getNetLines().keys());
  keptLines.remove(removedLine);
  QSet<Uuid> keptPoints;
  for (BI_NetPoint* point : segment->getNetPoints()) {
    if (!(point->getNetLines() - QSet<BI_NetLine*>{line}).isEmpty()) {
      keptPoints.insert(point->getUuid());
    }
  }
  CmdRemoveBoardItems cmd(board);
  cmd.removeNetLines({line});
  ASSERT_TRUE(cmd.execute());

  // Verify that the UUIDs have not changed.
  QSet<Uuid> lines;
  QSet<Uuid> points;
  for (BI_NetSegment* s : board.getNetSegments()) {
    lines |= Toolbox::toSet(s->getNetLines().keys());
    points |= Toolbox::toSet(s->getNetPoints().keys());
  }
  EXPECT_FALSE(lines.contains(removedLine));
  EXPECT_TRUE(lines.contains(keptLines));
  EXPECT_TRUE(points.contains(keptPoints));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
