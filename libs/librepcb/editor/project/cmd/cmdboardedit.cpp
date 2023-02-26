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
#include "cmdboardedit.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardlayerstack.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdBoardEdit::CmdBoardEdit(Board& board) noexcept
  : UndoCommand(tr("Modify Board Setup")),
    mBoard(board),
    mOldName(mBoard.getName()),
    mNewName(mOldName),
    mOldInnerLayerCount(mBoard.getLayerStack().getInnerLayerCount()),
    mNewInnerLayerCount(mOldInnerLayerCount),
    mOldDesignRules(mBoard.getDesignRules()),
    mNewDesignRules(mOldDesignRules) {
}

CmdBoardEdit::~CmdBoardEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdBoardEdit::setName(const ElementName& name) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewName = name;
}

void CmdBoardEdit::setInnerLayerCount(int count) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewInnerLayerCount = count;
}

void CmdBoardEdit::setDesignRules(const BoardDesignRules& rules) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDesignRules = rules;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardEdit::performExecute() {
  performRedo();  // can throw

  if (mNewName != mOldName) return true;
  if (mNewInnerLayerCount != mOldInnerLayerCount) return true;
  if (mNewDesignRules != mOldDesignRules) return true;
  return false;
}

void CmdBoardEdit::performUndo() {
  mBoard.setName(mOldName);
  mBoard.getLayerStack().setInnerLayerCount(mOldInnerLayerCount);
  mBoard.setDesignRules(mOldDesignRules);
}

void CmdBoardEdit::performRedo() {
  mBoard.setName(mNewName);
  mBoard.getLayerStack().setInnerLayerCount(mNewInnerLayerCount);
  mBoard.setDesignRules(mNewDesignRules);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
