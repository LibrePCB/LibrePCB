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
#include "cmdsplitnetline.h"

#include <librepcb/common/scopeguard.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentaddelements.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremoveelements.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_netpoint.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

CmdSplitNetLine::CmdSplitNetLine(BI_NetLine& netline, Point& pos)
  noexcept : UndoCommandGroup(tr("Split netline")),
  mOldNetLine(netline),
  mSplitPosition(pos) {
  mSplitPoint = new BI_NetPoint(mOldNetLine.getNetSegment(), mSplitPosition);
}

CmdSplitNetLine::~CmdSplitNetLine() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSplitNetLine::performExecute() {
  auto undoScopeGuard = scopeGuard([&]() { performUndo(); });

  QScopedPointer<CmdBoardNetSegmentAddElements>
      cmdAdd(new CmdBoardNetSegmentAddElements(mOldNetLine.getNetSegment()));
  cmdAdd->addNetPoint(*mSplitPoint);
  cmdAdd->addNetLine(*mSplitPoint, mOldNetLine.getStartPoint(),
                     mOldNetLine.getLayer(), mOldNetLine.getWidth());
  cmdAdd->addNetLine(*mSplitPoint, mOldNetLine.getEndPoint(),
                     mOldNetLine.getLayer(), mOldNetLine.getWidth());
  QScopedPointer<CmdBoardNetSegmentRemoveElements> cmdRemove(
        new CmdBoardNetSegmentRemoveElements(mOldNetLine.getNetSegment()));
  cmdRemove->removeNetLine(mOldNetLine);
  execNewChildCmd(cmdAdd.take()); // can throw
  execNewChildCmd(cmdRemove.take()); // can throw

  undoScopeGuard.dismiss();  // no undo required
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
