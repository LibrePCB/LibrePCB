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
#include "cmdfootprintstroketextremove.h"

#include "../items/bi_footprint.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdFootprintStrokeTextRemove::CmdFootprintStrokeTextRemove(
    BI_Footprint& footprint, BI_StrokeText& text) noexcept
  : UndoCommand(tr("Remove footprint text")),
    mFootprint(footprint),
    mText(text) {
}

CmdFootprintStrokeTextRemove::~CmdFootprintStrokeTextRemove() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdFootprintStrokeTextRemove::performExecute() {
  performRedo();  // can throw

  return true;
}

void CmdFootprintStrokeTextRemove::performUndo() {
  mFootprint.addStrokeText(mText);  // can throw
}

void CmdFootprintStrokeTextRemove::performRedo() {
  mFootprint.removeStrokeText(mText);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
