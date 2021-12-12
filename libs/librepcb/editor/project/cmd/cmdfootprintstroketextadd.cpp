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
#include "cmdfootprintstroketextadd.h"

#include <librepcb/core/project/board/items/bi_footprint.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdFootprintStrokeTextAdd::CmdFootprintStrokeTextAdd(
    BI_Footprint& footprint, BI_StrokeText& text) noexcept
  : UndoCommand(tr("Add footprint text")), mFootprint(footprint), mText(text) {
}

CmdFootprintStrokeTextAdd::~CmdFootprintStrokeTextAdd() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdFootprintStrokeTextAdd::performExecute() {
  performRedo();  // can throw

  return true;
}

void CmdFootprintStrokeTextAdd::performUndo() {
  mFootprint.removeStrokeText(mText);  // can throw
}

void CmdFootprintStrokeTextAdd::performRedo() {
  mFootprint.addStrokeText(mText);  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
