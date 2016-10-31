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
#include <QtCore>
#include "cmdflipselectedboarditems.h"
#include <librepcbcommon/scopeguard.h>
#include <librepcbcommon/gridproperties.h>
#include <librepcbcommon/boardlayer.h>
#include <librepcblibrary/pkg/footprintpad.h>
#include <librepcbproject/project.h>
#include <librepcbproject/boards/board.h>
#include <librepcbproject/boards/boardlayerstack.h>
#include <librepcbproject/boards/items/bi_device.h>
#include <librepcbproject/boards/items/bi_footprint.h>
#include <librepcbproject/boards/items/bi_footprintpad.h>
#include <librepcbproject/boards/items/bi_netpoint.h>
#include <librepcbproject/boards/items/bi_netline.h>
#include <librepcbproject/boards/items/bi_via.h>
#include <librepcbproject/boards/cmd/cmddeviceinstanceedit.h>
#include <librepcbproject/boards/cmd/cmdboardviaedit.h>
#include <librepcbproject/boards/cmd/cmdboardnetpointedit.h>
#include <librepcbproject/boards/cmd/cmdboardnetlineremove.h>
#include <librepcbproject/boards/cmd/cmdboardnetlineadd.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdFlipSelectedBoardItems::CmdFlipSelectedBoardItems(Board& board, Qt::Orientation orientation) noexcept :
    UndoCommandGroup(tr("Flip Board Elements")), mBoard(board), mOrientation(orientation)
{
}

CmdFlipSelectedBoardItems::~CmdFlipSelectedBoardItems() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdFlipSelectedBoardItems::performExecute() throw (Exception)
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // get all selected items
    QList<BI_Base*> items = mBoard.getSelectedItems(true, false, true, true, false, false,
                                                    false, false, true, true, true, false);

    // no items selected --> nothing to do here
    if (items.isEmpty()) {
        undoScopeGuard.dismiss();
        return false;
    }

    // find the center of all elements
    Point center = Point(0, 0);
    foreach (BI_Base* item, items) {
        center += item->getPosition();
    }
    center /= items.count();
    center.mapToGrid(mBoard.getGridProperties().getInterval());

    // TODO: make this feature more sophisticated!

    // find all bounding netpoints
    /*QList<BI_NetPoint*> netpoints;
    QList<BI_NetPoint*> boundingNetPoints;
    foreach (BI_Base* item, items) {
        if (item->getType() == BI_Base::Type_t::NetPoint) {
            BI_NetPoint* netpoint = dynamic_cast<BI_NetPoint*>(item); Q_ASSERT(netpoint);
            bool attachedToUnselectedPad = (netpoint->isAttachedToPad() && !netpoint->getFootprintPad()->isSelected());
            bool attachedToUnselectedVia = (netpoint->isAttachedToVia() && !netpoint->getVia()->isSelected());
            bool attachedToUnselectedLine = false;
            foreach (const BI_NetLine* netline, netpoint->getLines()) {
                if (!netline->isSelected()) {
                    attachedToUnselectedLine = true;
                    break;
                }
            }
            if (attachedToUnselectedPad || attachedToUnselectedVia || attachedToUnselectedLine) {
                boundingNetPoints.append(netpoint);
            }
            netpoints.append(netpoint);
        }
    }*/

    // disconnect all netlines
    /*QList<BI_NetLine*> netlines;
    foreach (BI_Base* item, items) {
        if (item->getType() == BI_Base::Type_t::NetLine) {
            BI_NetLine* netline = dynamic_cast<BI_NetLine*>(item); Q_ASSERT(netline);
            execNewChildCmd(new CmdBoardNetLineRemove(*netline)); // can throw
            netlines.append(netline);
        }
    }*/

    // disconnect all netpoints from pads/vias and change their layer and position
    /*foreach (BI_NetPoint* netpoint, netpoints) {
        if (!boundingNetPoints.contains(netpoint)) {
            if (netpoint->isAttached()) {
                CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*netpoint);
                cmd->setPadToAttach(nullptr);
                cmd->setViaToAttach(nullptr);
                execNewChildCmd(cmd); // can throw
            }
            BoardLayer* mirroredLayer = mBoard.getLayerStack().getBoardLayer(netpoint->getLayer().getMirroredLayerId());
            if (!mirroredLayer) throw LogicError(__FILE__, __LINE__);
            CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*netpoint);
            cmd->setLayer(*mirroredLayer);
            cmd->setPosition(netpoint->getPosition().mirrored(mOrientation, center), false);
            execNewChildCmd(cmd); // can throw
        }
    }*/

    // move all vias
    foreach (BI_Base* item, items) {
        if (item->getType() == BI_Base::Type_t::Via) {
            BI_Via* via = dynamic_cast<BI_Via*>(item); Q_ASSERT(via);
            CmdBoardViaEdit* cmd = new CmdBoardViaEdit(*via);
            cmd->setPosition(via->getPosition().mirrored(mOrientation, center), false);
            execNewChildCmd(cmd); // can throw
        }
    }

    // flip all device instances
    foreach (BI_Base* item, items) {
        if (item->getType() == BI_Base::Type_t::Footprint) {
            BI_Footprint* footprint = dynamic_cast<BI_Footprint*>(item); Q_ASSERT(footprint);
            CmdDeviceInstanceEdit* cmd = new CmdDeviceInstanceEdit(footprint->getDeviceInstance());
            cmd->mirror(center, mOrientation, false); // can throw
            execNewChildCmd(cmd); // can throw
        }
    }

    // reconnect all netlines
    /*foreach (BI_NetLine* netline, netlines) {
        if (!boundingNetPoints.contains(&netline->getStartPoint()) && !boundingNetPoints.contains(&netline->getEndPoint())) {
            execNewChildCmd(new CmdBoardNetLineAdd(*netline)); // can throw
        }
    }*/

    undoScopeGuard.dismiss(); // no undo required
    return (getChildCount() > 0);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void CmdFlipSelectedBoardItems::flipDevice(BI_Device& device, const Point& center) throw (Exception)
{
    // disconnect all netpoints/netlines
    foreach (BI_FootprintPad* pad, device.getFootprint().getPads()) {
        foreach (BI_NetPoint* netpoint, pad->getNetPoints()) {
            CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*netpoint);
            cmd->setPadToAttach(nullptr);
            execNewChildCmd(cmd); // can throw
        }
    }

    // flip device
    CmdDeviceInstanceEdit* cmd = new CmdDeviceInstanceEdit(device);
    cmd->mirror(center, mOrientation, false); // can throw
    execNewChildCmd(cmd); // can throw

    // reconnect all netpoints/netlines and change netline layers if required
    foreach (BI_FootprintPad* pad, device.getFootprint().getPads()) {
        if (pad->getLibPad().getTechnology() == library::FootprintPad::Technology_t::SMT) {
            // netline/netpoint must be flipped too (place via and change layer)

        }
        foreach (BI_NetPoint* netpoint, pad->getNetPoints()) {
            CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*netpoint);
            cmd->setPadToAttach(pad);
            execNewChildCmd(cmd); // can throw
        }
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
