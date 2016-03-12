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
#include "cmdmoveselectedboarditems.h"
#include <librepcbcommon/gridproperties.h>
#include <librepcbproject/project.h>
#include <librepcbproject/boards/board.h>
#include <librepcbproject/boards/items/bi_device.h>
#include <librepcbproject/boards/items/bi_footprint.h>
#include <librepcbproject/boards/cmd/cmddeviceinstanceedit.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdMoveSelectedBoardItems::CmdMoveSelectedBoardItems(Board& board, const Point& startPos) noexcept :
    UndoCommandGroup(tr("Move Board Elements")),
    mBoard(board), mStartPos(startPos), mDeltaPos(0, 0)
{
    // get all selected items
    QList<BI_Base*> items = mBoard.getSelectedItems(false /*true, false, true, false, false,
                                                    false, false, false, false, false*/);

    foreach (BI_Base* item, items) {
        switch (item->getType())
        {
            case BI_Base::Type_t::Footprint: {
                BI_Footprint* footprint = dynamic_cast<BI_Footprint*>(item); Q_ASSERT(footprint);
                BI_Device& device = footprint->getDeviceInstance();
                CmdDeviceInstanceEdit* cmd = new CmdDeviceInstanceEdit(device);
                mDeviceEditCmds.append(cmd);
                break;
            }
            default: {
                qCritical() << "Unknown board item type:" << static_cast<int>(item->getType());
                break;
            }
        }
    }
}

CmdMoveSelectedBoardItems::~CmdMoveSelectedBoardItems() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void CmdMoveSelectedBoardItems::setCurrentPosition(const Point& pos) noexcept
{
    Point delta = pos - mStartPos;
    delta.mapToGrid(mBoard.getGridProperties().getInterval());

    if (delta != mDeltaPos) {
        // move selected elements
        foreach (CmdDeviceInstanceEdit* cmd, mDeviceEditCmds) {
            cmd->setDeltaToStartPos(delta, true);
        }
        mDeltaPos = delta;
    }
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdMoveSelectedBoardItems::performExecute() throw (Exception)
{
    if (mDeltaPos.isOrigin()) {
        // no movement required --> discard all move commands
        qDeleteAll(mDeviceEditCmds);    mDeviceEditCmds.clear();
        return false;
    }

    foreach (CmdDeviceInstanceEdit* cmd, mDeviceEditCmds) {
        appendChild(cmd); // can throw
    }

    // execute all child commands
    return UndoCommandGroup::performExecute(); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
