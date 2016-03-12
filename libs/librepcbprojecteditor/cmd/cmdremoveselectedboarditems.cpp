/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
#include <QtCore>
#include "cmdremoveselectedboarditems.h"
#include <librepcbproject/project.h>
#include <librepcbproject/boards/board.h>
#include <librepcbproject/boards/items/bi_footprint.h>
#include <librepcbproject/boards/cmd/cmddeviceinstanceremove.h>
#include "cmdremoveunusednetsignals.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdRemoveSelectedBoardItems::CmdRemoveSelectedBoardItems(Board& board) noexcept :
    UndoCommandGroup(tr("Remove Board Elements")), mBoard(board)
{
}

CmdRemoveSelectedBoardItems::~CmdRemoveSelectedBoardItems() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdRemoveSelectedBoardItems::performExecute() throw (Exception)
{
    // TODO: use scope guard
    try
    {
        return buildAndExecuteChildCommands(); // can throw
    }
    catch (Exception&)
    {
        try
        {
            // undo all already executed child commands
            UndoCommandGroup::performUndo(); // can throw
        }
        catch (Exception&)
        {
            qFatal("Internal Fatal Error");
        }
        throw;
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool CmdRemoveSelectedBoardItems::buildAndExecuteChildCommands() throw (Exception)
{
    // get all selected items
    QList<BI_Base*> items = mBoard.getSelectedItems(false /*true, false, true, false, false,
                                                    false, false, false, false, false*/);

    // no items selected --> nothing to do here
    if (items.isEmpty()) {
        return false;
    }

    // clear selection because these items will be removed now
    mBoard.clearSelection();

    // remove all device instances
    foreach (BI_Base* item, items) {
        if (item->getType() == BI_Base::Type_t::Footprint) {
            BI_Footprint* footprint = dynamic_cast<BI_Footprint*>(item); Q_ASSERT(footprint);
            BI_Device& device = footprint->getDeviceInstance();
            execNewChildCmd(new CmdDeviceInstanceRemove(mBoard, device)); // can throw
        }
    }

    // remove netsignals which are no longer required
    execNewChildCmd(new CmdRemoveUnusedNetSignals(mBoard.getProject().getCircuit())); // can throw

    return (getChildCount() > 0);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
