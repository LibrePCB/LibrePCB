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
#include <librepcbcommon/scopeguard.h>
#include <librepcbproject/project.h>
#include <librepcbproject/boards/board.h>
#include <librepcbproject/boards/items/bi_device.h>
#include <librepcbproject/boards/items/bi_footprint.h>
#include <librepcbproject/boards/items/bi_via.h>
#include <librepcbproject/boards/items/bi_netline.h>
#include <librepcbproject/boards/items/bi_netpoint.h>
#include <librepcbproject/boards/cmd/cmddeviceinstanceremove.h>
#include <librepcbproject/boards/cmd/cmdboardviaremove.h>
#include <librepcbproject/boards/cmd/cmdboardnetlineadd.h>
#include <librepcbproject/boards/cmd/cmdboardnetlineremove.h>
#include <librepcbproject/boards/cmd/cmdboardnetpointremove.h>
#include <librepcbproject/boards/cmd/cmdboardnetpointedit.h>

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
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // get all selected items
    QList<BI_Base*> items = mBoard.getSelectedItems(true, false, true, true, true, true,
                                                    true, true, true, true, true, false);

    // clear selection because these items will be removed now
    mBoard.clearSelection();

    // remove all netlines
    foreach (BI_Base* item, items) {
        if (item->getType() == BI_Base::Type_t::NetLine) {
            BI_NetLine* netline = dynamic_cast<BI_NetLine*>(item); Q_ASSERT(netline);
            execNewChildCmd(new CmdBoardNetLineRemove(*netline)); // can throw
        }
    }

    // remove all netpoints
    foreach (BI_Base* item, items) {
        if (item->getType() == BI_Base::Type_t::NetPoint) {
            BI_NetPoint* netpoint = dynamic_cast<BI_NetPoint*>(item); Q_ASSERT(netpoint);
            // TODO: this code does not work correctly in all possible cases!
            if (netpoint->getLines().count() == 0) {
                execNewChildCmd(new CmdBoardNetPointRemove(*netpoint)); // can throw
            } else if (netpoint->isAttached()) {
                // disconnect all netlines
                QList<BI_NetLine*> netlines = netpoint->getLines();
                foreach (BI_NetLine* netline, netlines) {
                    execNewChildCmd(new CmdBoardNetLineRemove(*netline)); // can throw
                }
                // detach netpoint from footprint pad
                CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*netpoint);
                cmd->setPadToAttach(nullptr);
                cmd->setViaToAttach(nullptr);
                execNewChildCmd(cmd); // can throw
                // reconnect all netlines
                foreach (BI_NetLine* netline, netlines) {
                    execNewChildCmd(new CmdBoardNetLineAdd(*netline)); // can throw
                }
            }
        }
    }

    // remove all vias
    foreach (BI_Base* item, items) {
        if (item->getType() == BI_Base::Type_t::Via) {
            BI_Via* via = dynamic_cast<BI_Via*>(item); Q_ASSERT(via);
            execNewChildCmd(new CmdBoardViaRemove(*via)); // can throw
        }
    }

    // remove all device instances
    foreach (BI_Base* item, items) {
        if (item->getType() == BI_Base::Type_t::Footprint) {
            BI_Footprint* footprint = dynamic_cast<BI_Footprint*>(item); Q_ASSERT(footprint);
            BI_Device& device = footprint->getDeviceInstance();
            execNewChildCmd(new CmdDeviceInstanceRemove(mBoard, device)); // can throw
        }
    }

    undoScopeGuard.dismiss(); // no undo required
    return (getChildCount() > 0);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
