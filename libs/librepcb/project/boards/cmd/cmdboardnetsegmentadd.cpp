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
#include "cmdboardnetsegmentadd.h"

#include "../board.h"
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

CmdBoardNetSegmentAdd::CmdBoardNetSegmentAdd(BI_NetSegment& segment) noexcept
  : UndoCommand(tr("Add net segment")),
    mBoard(segment.getBoard()),
    mNetSignal(segment.getNetSignal()),
    mNetSegment(&segment) {
}

CmdBoardNetSegmentAdd::CmdBoardNetSegmentAdd(Board&     board,
                                             NetSignal& netsignal) noexcept
  : UndoCommand(tr("Add net segment")),
    mBoard(board),
    mNetSignal(netsignal),
    mNetSegment(nullptr) {
}

CmdBoardNetSegmentAdd::~CmdBoardNetSegmentAdd() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardNetSegmentAdd::performExecute() {
  if (!mNetSegment) {
    // create new net segment
    mNetSegment = new BI_NetSegment(mBoard, mNetSignal);  // can throw
  }

  performRedo();  // can throw

  return true;
}

void CmdBoardNetSegmentAdd::performUndo() {
  mBoard.removeNetSegment(*mNetSegment);  // can throw
}

void CmdBoardNetSegmentAdd::performRedo() {
  mBoard.addNetSegment(*mNetSegment);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
