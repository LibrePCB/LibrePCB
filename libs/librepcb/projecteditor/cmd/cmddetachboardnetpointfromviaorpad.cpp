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
#include <librepcb/project/boards/cmd/cmdboardnetpointedit.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentadd.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremove.h>
#include <librepcb/project/boards/cmd/cmdboardnetsegmentremoveelements.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

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

bool CmdDetachBoardNetPointFromViaOrPad::performExecute()
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

void CmdDetachBoardNetPointFromViaOrPad::detachNetPoint()
{
    // disconnect whole netsegment
    execNewChildCmd(new CmdBoardNetSegmentRemove(mNetPoint.getNetSegment()));

    // detach netpoint from via or pad
    CmdBoardNetPointEdit* cmd = new CmdBoardNetPointEdit(mNetPoint);
    cmd->setViaToAttach(nullptr);
    cmd->setPadToAttach(nullptr);
    execNewChildCmd(cmd); // can throw

    // re-connect whole netsegment
    execNewChildCmd(new CmdBoardNetSegmentAdd(mNetPoint.getNetSegment()));
}

void CmdDetachBoardNetPointFromViaOrPad::removeNetPointWithAllNetlines()
{
    // remove the netpoint itself
    QScopedPointer<CmdBoardNetSegmentRemoveElements> cmd(
        new CmdBoardNetSegmentRemoveElements(mNetPoint.getNetSegment()));
    cmd->removeNetPoint(mNetPoint);

    // remove all connected netlines
    foreach (BI_NetLine* netline, mNetPoint.getLines()) { Q_ASSERT(netline);
        cmd->removeNetLine(*netline);
        BI_NetPoint* other = netline->getOtherPoint(mNetPoint); Q_ASSERT(other);
        if (other->getLines().count() <= 1) { cmd->removeNetPoint(*other); }
    }

    execNewChildCmd(cmd.take()); // can throw
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
