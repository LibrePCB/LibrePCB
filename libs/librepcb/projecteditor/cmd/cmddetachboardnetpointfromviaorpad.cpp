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
#include "cmddetachboardnetpointfromviaorpad.h"
#include <librepcb/common/scopeguard.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/items/bi_via.h>
#include <librepcb/project/boards/items/bi_netpoint.h>
#include <librepcb/project/boards/items/bi_netline.h>
#include <librepcb/project/boards/cmd/cmdboardviaremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointedit.h>
#include <librepcb/project/boards/cmd/cmdboardnetpointremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetlineadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetlineremove.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdDetachBoardNetPointFromViaOrPad::CmdDetachBoardNetPointFromViaOrPad(BI_NetPoint& p) noexcept :
    UndoCommandGroup(tr("Detach netpoint from via or pad")),
    mNetPoint(p)
{
}

CmdDetachBoardNetPointFromViaOrPad::~CmdDetachBoardNetPointFromViaOrPad() noexcept
{
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdDetachBoardNetPointFromViaOrPad::performExecute() throw (Exception)
{
    // if an error occurs, undo all already executed child commands
    auto undoScopeGuard = scopeGuard([&](){performUndo();});

    // decide what to do with the netpoint
    if (mNetPoint.getLines().count() <= 1) {
        removeNetPointWithAllNetlines(); // can throw
    } else {
        detachNetPoint(); // can throw
    }

    undoScopeGuard.dismiss(); // no undo required
    return (getChildCount() > 0);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void CmdDetachBoardNetPointFromViaOrPad::detachNetPoint() throw (Exception)
{
    // disconnect all netlines
    QList<BI_NetLine*> netlines = mNetPoint.getLines();
    foreach (BI_NetLine* netline, netlines) {
        execNewChildCmd(new CmdBoardNetLineRemove(*netline));
    }

    // detach netpoint from via or pad
    CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(mNetPoint);
    cmd->setViaToAttach(nullptr);
    cmd->setPadToAttach(nullptr);
    execNewChildCmd(cmd); // can throw

    // re-connect all netlines
    foreach (BI_NetLine* netline, netlines) {
        execNewChildCmd(new CmdBoardNetLineAdd(*netline));
    }
}

void CmdDetachBoardNetPointFromViaOrPad::removeNetPointWithAllNetlines() throw (Exception)
{
    // remove all connected netlines
    foreach (BI_NetLine* netline, mNetPoint.getLines()) { Q_ASSERT(netline);
        removeNetLineWithUnusedNetpoints(*netline); // can throw
    }

    // the netpoint itself should be already removed now
    Q_ASSERT(!mNetPoint.isAddedToBoard());
}

void CmdDetachBoardNetPointFromViaOrPad::removeNetLineWithUnusedNetpoints(BI_NetLine& l) throw (Exception)
{
    // remove the netline itself
    execNewChildCmd(new CmdBoardNetLineRemove(l)); // can throw

    // remove endpoints of the netline which are no longer required
    removeNetpointIfUnused(l.getStartPoint()); // can throw
    removeNetpointIfUnused(l.getEndPoint()); // can throw
}

void CmdDetachBoardNetPointFromViaOrPad::removeNetpointIfUnused(BI_NetPoint& p) throw (Exception)
{
    if (p.getLines().count() == 0) {
        execNewChildCmd(new CmdBoardNetPointRemove(p)); // can throw
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
