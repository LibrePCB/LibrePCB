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
#include "cmdboardstroketextremove.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_stroketext.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdBoardStrokeTextRemove::CmdBoardStrokeTextRemove(BI_StrokeText& text) noexcept
  : UndoCommand(tr("Remove text from board")),
    mBoard(text.getBoard()),
    mText(text) {
}

CmdBoardStrokeTextRemove::~CmdBoardStrokeTextRemove() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardStrokeTextRemove::performExecute() {
  performRedo();  // can throw

  return true;
}

void CmdBoardStrokeTextRemove::performUndo() {
  mBoard.addStrokeText(mText);  // can throw
}

void CmdBoardStrokeTextRemove::performRedo() {
  mBoard.removeStrokeText(mText);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
