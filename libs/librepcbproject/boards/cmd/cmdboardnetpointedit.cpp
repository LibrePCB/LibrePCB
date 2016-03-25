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
#include "cmdboardnetpointedit.h"
#include <librepcbcommon/scopeguardlist.h>
#include "../items/bi_netpoint.h"
#include "../items/bi_footprintpad.h"
#include "../items/bi_via.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdBoardNetPointEdit::CmdBoardNetPointEdit(BI_NetPoint& point) noexcept :
    UndoCommand(tr("Edit netpoint")), mNetPoint(point),
    mOldLayer(&point.getLayer()), mNewLayer(mOldLayer),
    mOldNetSignal(&point.getNetSignal()), mNewNetSignal(mOldNetSignal),
    mOldFootprintPad(point.getFootprintPad()), mNewFootprintPad(mOldFootprintPad),
    mOldVia(point.getVia()), mNewVia(mOldVia),
    mOldPos(point.getPosition()), mNewPos(mOldPos)
{
}

CmdBoardNetPointEdit::~CmdBoardNetPointEdit() noexcept
{
    if (!wasEverExecuted()) {
        mNetPoint.setPosition(mOldPos);
    }
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdBoardNetPointEdit::setLayer(BoardLayer& layer) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewLayer = &layer;
}

void CmdBoardNetPointEdit::setNetSignal(NetSignal& netsignal) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewNetSignal = &netsignal;
}

void CmdBoardNetPointEdit::setPadToAttach(BI_FootprintPad* pad) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewFootprintPad = pad;
    if (pad) mNewPos = pad->getPosition();
}

void CmdBoardNetPointEdit::setViaToAttach(BI_Via* via) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewVia = via;
    if (via) mNewPos = via->getPosition();
}

void CmdBoardNetPointEdit::setPosition(const Point& pos, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewPos = pos;
    if (immediate) mNetPoint.setPosition(mNewPos);
}

void CmdBoardNetPointEdit::setDeltaToStartPos(const Point& deltaPos, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewPos = mOldPos + deltaPos;
    if (immediate) mNetPoint.setPosition(mNewPos);
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdBoardNetPointEdit::performExecute() throw (Exception)
{
    performRedo(); // can throw

    return true; // TODO: determine if the netpoint was really modified
}

void CmdBoardNetPointEdit::performUndo() throw (Exception)
{
    ScopeGuardList sgl;
    mNetPoint.setLayer(*mOldLayer); // can throw
    sgl.add([&](){mNetPoint.setLayer(*mNewLayer);});
    mNetPoint.setNetSignal(*mOldNetSignal); // can throw
    sgl.add([&](){mNetPoint.setNetSignal(*mNewNetSignal);});
    mNetPoint.setPadToAttach(mOldFootprintPad); // can throw
    sgl.add([&](){mNetPoint.setPadToAttach(mNewFootprintPad);});
    mNetPoint.setViaToAttach(mOldVia); // can throw
    sgl.add([&](){mNetPoint.setViaToAttach(mNewVia);});
    mNetPoint.setPosition(mOldPos);
    sgl.dismiss();
}

void CmdBoardNetPointEdit::performRedo() throw (Exception)
{
    ScopeGuardList sgl;
    mNetPoint.setLayer(*mNewLayer); // can throw
    sgl.add([&](){mNetPoint.setLayer(*mOldLayer);});
    mNetPoint.setNetSignal(*mNewNetSignal); // can throw
    sgl.add([&](){mNetPoint.setNetSignal(*mOldNetSignal);});
    mNetPoint.setPadToAttach(mNewFootprintPad); // can throw
    sgl.add([&](){mNetPoint.setPadToAttach(mOldFootprintPad);});
    mNetPoint.setViaToAttach(mNewVia); // can throw
    sgl.add([&](){mNetPoint.setViaToAttach(mOldVia);});
    mNetPoint.setPosition(mNewPos);
    sgl.dismiss();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
