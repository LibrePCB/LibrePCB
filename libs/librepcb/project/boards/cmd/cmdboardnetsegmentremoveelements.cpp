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
#include "cmdboardnetsegmentremoveelements.h"

#include "../board.h"
#include "../items/bi_netline.h"
#include "../items/bi_netpoint.h"
#include "../items/bi_netsegment.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdBoardNetSegmentRemoveElements::CmdBoardNetSegmentRemoveElements(
    BI_NetSegment& segment) noexcept
  : UndoCommand(tr("Remove net segment elements")), mNetSegment(segment) {
}

CmdBoardNetSegmentRemoveElements::~CmdBoardNetSegmentRemoveElements() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CmdBoardNetSegmentRemoveElements::removeVia(BI_Via& via) {
  mVias.append(&via);
}

void CmdBoardNetSegmentRemoveElements::removeNetPoint(BI_NetPoint& netpoint) {
  mNetPoints.append(&netpoint);
}

void CmdBoardNetSegmentRemoveElements::removeNetLine(BI_NetLine& netline) {
  mNetLines.append(&netline);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardNetSegmentRemoveElements::performExecute() {
  performRedo();  // can throw

  return true;
}

void CmdBoardNetSegmentRemoveElements::performUndo() {
  mNetSegment.addElements(mVias, mNetPoints, mNetLines);  // can throw
}

void CmdBoardNetSegmentRemoveElements::performRedo() {
  mNetSegment.removeElements(mVias, mNetPoints, mNetLines);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
