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
#include "cmdfootprintstroketextsreset.h"

#include "cmdfootprintstroketextadd.h"
#include "cmdfootprintstroketextremove.h"

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

CmdFootprintStrokeTextsReset::CmdFootprintStrokeTextsReset(
    BI_Footprint& footprint) noexcept
  : UndoCommandGroup(tr("Reset footprint texts")), mFootprint(footprint) {
}

CmdFootprintStrokeTextsReset::~CmdFootprintStrokeTextsReset() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdFootprintStrokeTextsReset::performExecute() {
  // Remove all texts
  foreach (BI_StrokeText* text, mFootprint.getStrokeTexts()) {
    appendChild(new CmdFootprintStrokeTextRemove(mFootprint, *text));
  }

  // Create new texts
  for (const StrokeText& text : mFootprint.getDefaultStrokeTexts()) {
    appendChild(new CmdFootprintStrokeTextAdd(
        mFootprint, *new BI_StrokeText(mFootprint.getBoard(), text)));
  }

  // execute all child commands
  return UndoCommandGroup::performExecute();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
