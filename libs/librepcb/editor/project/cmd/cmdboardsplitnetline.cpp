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
#include "cmdboardsplitnetline.h"

#include "../../project/cmd/cmdboardnetsegmentaddelements.h"
#include "../../project/cmd/cmdboardnetsegmentremove.h"
#include "../../project/cmd/cmdboardnetsegmentremoveelements.h"

#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netpoint.h>
#include <librepcb/core/utils/scopeguard.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

CmdBoardSplitNetLine::CmdBoardSplitNetLine(BI_NetLine& netline,
                                           const Point& pos) noexcept
  : UndoCommandGroup(tr("Split trace")), mOldNetLine(netline) {
  mSplitPoint = new BI_NetPoint(mOldNetLine.getNetSegment(), pos);
}

CmdBoardSplitNetLine::~CmdBoardSplitNetLine() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardSplitNetLine::performExecute() {
  QScopedPointer<CmdBoardNetSegmentAddElements> cmdAdd(
      new CmdBoardNetSegmentAddElements(mOldNetLine.getNetSegment()));
  cmdAdd->addNetPoint(*mSplitPoint);
  cmdAdd->addNetLine(*mSplitPoint, mOldNetLine.getStartPoint(),
                     mOldNetLine.getLayer(), mOldNetLine.getWidth());
  cmdAdd->addNetLine(*mSplitPoint, mOldNetLine.getEndPoint(),
                     mOldNetLine.getLayer(), mOldNetLine.getWidth());
  QScopedPointer<CmdBoardNetSegmentRemoveElements> cmdRemove(
      new CmdBoardNetSegmentRemoveElements(mOldNetLine.getNetSegment()));
  cmdRemove->removeNetLine(mOldNetLine);
  appendChild(cmdAdd.take());
  appendChild(cmdRemove.take());
  return UndoCommandGroup::performExecute();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
