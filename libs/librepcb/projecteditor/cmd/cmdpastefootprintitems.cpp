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
#include "cmdpastefootprintitems.h"

#include <librepcb/libraryeditor/pkg/footprintclipboarddata.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/cmd/cmdboardholeadd.h>
#include <librepcb/project/boards/cmd/cmdboardpolygonadd.h>
#include <librepcb/project/boards/cmd/cmdboardstroketextadd.h>
#include <librepcb/project/boards/items/bi_hole.h>
#include <librepcb/project/boards/items/bi_polygon.h>
#include <librepcb/project/boards/items/bi_stroketext.h>
#include <librepcb/project/project.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdPasteFootprintItems::CmdPasteFootprintItems(
    Board& board, std::unique_ptr<library::editor::FootprintClipboardData> data,
    const Point& posOffset) noexcept
  : UndoCommandGroup(tr("Paste Board Elements")),
    mProject(board.getProject()),
    mBoard(board),
    mData(std::move(data)),
    mPosOffset(posOffset) {
}

CmdPasteFootprintItems::~CmdPasteFootprintItems() noexcept {
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdPasteFootprintItems::performExecute() {
  // Notes:
  //
  //  - The graphics items of the added elements are selected immediately to
  //    allow dragging them afterwards.

  // Paste polygons
  for (const Polygon& polygon : mData->getPolygons()) {
    Polygon copy(Uuid::createRandom(), polygon);  // assign new UUID
    copy.setPath(copy.getPath().translated(mPosOffset));  // move
    BI_Polygon* item = new BI_Polygon(mBoard, copy);
    item->setSelected(true);
    appendChild(new CmdBoardPolygonAdd(*item));
  }

  // Paste stroke texts
  for (const StrokeText& text : mData->getStrokeTexts()) {
    StrokeText copy(Uuid::createRandom(), text);  // assign new UUID
    copy.setPosition(copy.getPosition() + mPosOffset);  // move
    BI_StrokeText* item = new BI_StrokeText(mBoard, copy);
    item->setSelected(true);
    appendChild(new CmdBoardStrokeTextAdd(*item));
  }

  // Paste holes
  for (const Hole& hole : mData->getHoles()) {
    Hole copy(Uuid::createRandom(), hole);  // assign new UUID
    copy.setPosition(copy.getPosition() + mPosOffset);  // move
    BI_Hole* item = new BI_Hole(mBoard, copy);
    item->setSelected(true);
    appendChild(new CmdBoardHoleAdd(*item));
  }

  return UndoCommandGroup::performExecute();  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
