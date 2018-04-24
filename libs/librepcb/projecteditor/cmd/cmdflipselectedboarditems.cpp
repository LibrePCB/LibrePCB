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
#include <librepcb/common/scopeguard.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/geometry/cmd/cmdstroketextedit.h>
#include <librepcb/common/geometry/cmd/cmdholeedit.h>
#include <librepcb/library/pkg/footprintpad.h>
#include <librepcb/project/project.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/boards/items/bi_footprint.h>
#include <librepcb/project/boards/items/bi_footprintpad.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/boards/items/bi_stroketext.h>
#include <librepcb/project/boards/items/bi_hole.h>
#include <librepcb/project/boards/cmd/cmddeviceinstanceedit.h>
#include <librepcb/project/boards/cmd/cmdboardviaedit.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointedit.h>
#include <librepcb/project/boards/boardselectionquery.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

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

bool CmdFlipSelectedBoardItems::performExecute()
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // get all selected items
    std::unique_ptr<BoardSelectionQuery> query(mBoard.createSelectionQuery());
    query->addSelectedFootprints();
    query->addSelectedVias();
    query->addSelectedBoardStrokeTexts();
    query->addSelectedFootprintStrokeTexts();
    query->addSelectedHoles();

    // find the center of all elements
    Point center = Point(0, 0);
    int count = 0;
    foreach (BI_Footprint* footprint, query->getFootprints()) {
        center += footprint->getPosition();
        ++count;
    }
    foreach (BI_Via* via, query->getVias()) {
        center += via->getPosition();
        ++count;
    }
    foreach (BI_StrokeText* text, query->getStrokeTexts()) {
        // do not count texts of footprints if the footprint is selected too
        if (!query->getFootprints().contains(text->getFootprint())) {
            center += text->getPosition();
            ++count;
        }
    }
    foreach (BI_Hole* hole, query->getHoles()) {
        center += hole->getPosition();
        ++count;
    }
    if (count > 0) {
        center /= count;
        center.mapToGrid(mBoard.getGridProperties().getInterval());
    } else {
        // no items selected --> nothing to do here
        undoScopeGuard.dismiss();
        return false;
    }

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
            BoardLayer* mirroredLayer = mBoard.getLayerStack().getLayer(netpoint->getLayer().getMirroredLayerId());
            if (!mirroredLayer) throw LogicError(__FILE__, __LINE__);
            CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(*netpoint);
            cmd->setLayer(*mirroredLayer);
            cmd->setPosition(netpoint->getPosition().mirrored(mOrientation, center), false);
            execNewChildCmd(cmd); // can throw
        }
    }*/

    // move all vias
    foreach (BI_Via* via, query->getVias()) { Q_ASSERT(via);
        CmdBoardViaEdit* cmd = new CmdBoardViaEdit(*via);
        cmd->setPosition(via->getPosition().mirrored(mOrientation, center), false);
        execNewChildCmd(cmd); // can throw
    }

    // flip all device instances
    foreach (BI_Footprint* footprint, query->getFootprints()) { Q_ASSERT(footprint);
        CmdDeviceInstanceEdit* cmd = new CmdDeviceInstanceEdit(footprint->getDeviceInstance());
        cmd->mirror(center, mOrientation, false); // can throw
        execNewChildCmd(cmd); // can throw
    }

    // flip all stroke texts
    foreach (BI_StrokeText* text, query->getStrokeTexts()) {
        CmdStrokeTextEdit* cmd = new CmdStrokeTextEdit(text->getText());
        cmd->mirror(center, mOrientation, false);
        execNewChildCmd(cmd); // can throw
    }

    // move all holes
    foreach (BI_Hole* hole, query->getHoles()) {
        CmdHoleEdit* cmd = new CmdHoleEdit(hole->getHole());
        cmd->setPosition(hole->getPosition().mirrored(mOrientation, center), false);
        execNewChildCmd(cmd); // can throw
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

void CmdFlipSelectedBoardItems::flipDevice(BI_Device& device, const Point& center)
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
        //if (pad->getLibPad().getTechnology() == library::FootprintPad::Technology_t::SMT) {
        //    // netline/netpoint must be flipped too (place via and change layer)
        //
        //}
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

} // namespace editor
} // namespace project
} // namespace librepcb
