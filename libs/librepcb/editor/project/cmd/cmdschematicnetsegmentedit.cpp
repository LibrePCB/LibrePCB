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
#include "cmdschematicnetsegmentedit.h"

#include <librepcb/core/project/schematic/items/si_netsegment.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdSchematicNetSegmentEdit::CmdSchematicNetSegmentEdit(
    SI_NetSegment& netsegment) noexcept
  : UndoCommand(tr("Edit net segment")),
    mNetSegment(netsegment),
    mOldNetSignal(&netsegment.getNetSignal()),
    mNewNetSignal(mOldNetSignal) {
}

CmdSchematicNetSegmentEdit::~CmdSchematicNetSegmentEdit() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdSchematicNetSegmentEdit::setNetSignal(NetSignal& netsignal) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewNetSignal = &netsignal;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdSchematicNetSegmentEdit::performExecute() {
  performRedo();  // can throw

  return (mNewNetSignal != mOldNetSignal);
}

void CmdSchematicNetSegmentEdit::performUndo() {
  mNetSegment.setNetSignal(*mOldNetSignal);  // can throw
}

void CmdSchematicNetSegmentEdit::performRedo() {
  mNetSegment.setNetSignal(*mNewNetSignal);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
