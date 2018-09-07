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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <memory>
#include <QtCore>
#include "cmdremoveselectedboarditems.h"
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardselectionquery.h>
#include "cmdremoveboarditems.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdRemoveSelectedBoardItems::CmdRemoveSelectedBoardItems(Board& board) noexcept :
    UndoCommand(tr("Remove Board Items")), mBoard(board)
{
}

CmdRemoveSelectedBoardItems::~CmdRemoveSelectedBoardItems() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdRemoveSelectedBoardItems::performExecute()
{
    // get all selected items
    std::unique_ptr<BoardSelectionQuery> query(mBoard.createSelectionQuery());
    query->addDeviceInstancesOfSelectedFootprints();
    query->addSelectedVias();
    query->addSelectedNetLines();
    query->addSelectedPlanes();
    query->addSelectedPolygons();
    query->addSelectedBoardStrokeTexts();
    query->addSelectedFootprintStrokeTexts();
    query->addSelectedHoles();

    // clear selection because these items will be removed now
    mBoard.clearSelection();

    // remove items
    mWrappedCommand.reset(new CmdRemoveBoardItems(mBoard));
    mWrappedCommand->removeDeviceInstances(query->getDeviceInstances());
    mWrappedCommand->removeVias(query->getVias());
    mWrappedCommand->removeNetLines(query->getNetLines());
    mWrappedCommand->removePlanes(query->getPlanes());
    mWrappedCommand->removePolygons(query->getPolygons());
    mWrappedCommand->removeStrokeTexts(query->getStrokeTexts());
    mWrappedCommand->removeHoles(query->getHoles());
    return mWrappedCommand->execute();
}

void CmdRemoveSelectedBoardItems::performUndo()
{
    mWrappedCommand->undo();
}

void CmdRemoveSelectedBoardItems::performRedo()
{
    mWrappedCommand->redo();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
