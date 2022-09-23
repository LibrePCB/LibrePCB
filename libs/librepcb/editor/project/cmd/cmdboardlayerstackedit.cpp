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
#include "cmdboardlayerstackedit.h"

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

CmdBoardLayerStackEdit::CmdBoardLayerStackEdit(
    BoardLayerStack& layerStack) noexcept
  : UndoCommand(tr("Modify board layer stack")),
    mLayerStack(layerStack),
    mOldInnerLayerCount(mLayerStack.getInnerLayerCount()),
    mNewInnerLayerCount(mOldInnerLayerCount) {
}

CmdBoardLayerStackEdit::~CmdBoardLayerStackEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdBoardLayerStackEdit::setInnerLayerCount(int count) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewInnerLayerCount = count;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardLayerStackEdit::performExecute() {
  performRedo();  // can throw

  if (mNewInnerLayerCount != mOldInnerLayerCount) return true;
  return false;
}

void CmdBoardLayerStackEdit::performUndo() {
  mLayerStack.setInnerLayerCount(mOldInnerLayerCount);
}

void CmdBoardLayerStackEdit::performRedo() {
  mLayerStack.setInnerLayerCount(mNewInnerLayerCount);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
