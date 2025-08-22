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
#include "cmdboardpadedit.h"

#include <librepcb/core/project/board/items/bi_pad.h>
#include <librepcb/core/types/layer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdBoardPadEdit::CmdBoardPadEdit(BI_Pad& pad) noexcept
  : UndoCommand(tr("Edit Pad")),
    mPad(pad),
    mOldPos(pad.getPosition()),
    mNewPos(mOldPos),
    mOldRotation(pad.getRotation()),
    mNewRotation(mOldRotation) {
  Q_ASSERT(pad.getNetSegment());  // Only board pads are mutable.
}

CmdBoardPadEdit::~CmdBoardPadEdit() noexcept {
  if (!wasEverExecuted()) {
    mPad.setPosition(mOldPos);
    mPad.setRotation(mOldRotation);
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdBoardPadEdit::setPosition(const Point& pos, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos = pos;
  if (immediate) mPad.setPosition(mNewPos);
}

void CmdBoardPadEdit::translate(const Point& deltaPos,
                                bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos += deltaPos;
  if (immediate) mPad.setPosition(mNewPos);
}

void CmdBoardPadEdit::snapToGrid(const PositiveLength& gridInterval,
                                 bool immediate) noexcept {
  setPosition(mNewPos.mappedToGrid(gridInterval), immediate);
}

void CmdBoardPadEdit::rotate(const Angle& angle, const Point& center,
                             bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos.rotate(angle, center);
  mNewRotation += angle;
  if (immediate) {
    mPad.setPosition(mNewPos);
    mPad.setRotation(mNewRotation);
  }
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardPadEdit::performExecute() {
  if (!mPad.getNetSegment()) {
    throw LogicError(__FILE__, __LINE__);  // Only board pads are mutable.
  }

  performRedo();  // can throw

  if (mNewPos != mOldPos) return true;
  if (mNewRotation != mOldRotation) return true;
  return false;
}

void CmdBoardPadEdit::performUndo() {
  mPad.setPosition(mOldPos);
  mPad.setRotation(mOldRotation);
}

void CmdBoardPadEdit::performRedo() {
  mPad.setPosition(mNewPos);
  mPad.setRotation(mNewRotation);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
