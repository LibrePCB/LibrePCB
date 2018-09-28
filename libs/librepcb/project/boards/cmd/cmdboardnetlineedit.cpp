/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "cmdboardnetlineedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdBoardNetLineEdit::CmdBoardNetLineEdit(BI_NetLine& netline) noexcept
  : UndoCommand(tr("Edit trace")),
    mNetLine(netline),
    mOldLayer(&netline.getLayer()),
    mNewLayer(mOldLayer),
    mOldWidth(netline.getWidth()),
    mNewWidth(mOldWidth) {
}

CmdBoardNetLineEdit::~CmdBoardNetLineEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdBoardNetLineEdit::setLayer(GraphicsLayer& layer) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewLayer = &layer;
}

void CmdBoardNetLineEdit::setWidth(const PositiveLength& width) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewWidth = width;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardNetLineEdit::performExecute() {
  performRedo();  // can throw

  return true;  // TODO: determine if the via was really modified
}

void CmdBoardNetLineEdit::performUndo() {
  mNetLine.setLayer(*mOldLayer);
  mNetLine.setWidth(mOldWidth);
}

void CmdBoardNetLineEdit::performRedo() {
  mNetLine.setLayer(*mNewLayer);
  mNetLine.setWidth(mNewWidth);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
