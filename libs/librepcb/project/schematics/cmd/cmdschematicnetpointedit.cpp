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
#include "cmdschematicnetpointedit.h"
#include <librepcb/common/scopeguardlist.h>
#include "../items/si_netpoint.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSchematicNetPointEdit::CmdSchematicNetPointEdit(SI_NetPoint& point) noexcept :
    UndoCommand(tr("Edit netpoint")), mNetPoint(point),
    mOldNetSignal(&point.getNetSignal()), mNewNetSignal(mOldNetSignal),
    mOldSymbolPin(point.getSymbolPin()), mNewSymbolPin(mOldSymbolPin),
    mOldPos(point.getPosition()), mNewPos(mOldPos)
{
}

CmdSchematicNetPointEdit::~CmdSchematicNetPointEdit() noexcept
{
    if (!wasEverExecuted()) {
        mNetPoint.setPosition(mOldPos);
    }
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdSchematicNetPointEdit::setNetSignal(NetSignal& netsignal) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewNetSignal = &netsignal;
}

void CmdSchematicNetPointEdit::setPinToAttach(SI_SymbolPin* pin) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewSymbolPin = pin;
}

void CmdSchematicNetPointEdit::setPosition(const Point& pos, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewPos = pos;
    if (immediate) mNetPoint.setPosition(mNewPos);
}

void CmdSchematicNetPointEdit::setDeltaToStartPos(const Point& deltaPos, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewPos = mOldPos + deltaPos;
    if (immediate) mNetPoint.setPosition(mNewPos);
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdSchematicNetPointEdit::performExecute() throw (Exception)
{
    performRedo(); // can throw

    return true; // TODO: determine if the netpoint was really modified
}

void CmdSchematicNetPointEdit::performUndo() throw (Exception)
{
    ScopeGuardList sgl;
    mNetPoint.setNetSignal(*mOldNetSignal); // can throw
    sgl.add([&](){mNetPoint.setNetSignal(*mNewNetSignal);});
    mNetPoint.setPinToAttach(mOldSymbolPin); // can throw
    sgl.add([&](){mNetPoint.setPinToAttach(mNewSymbolPin);});
    mNetPoint.setPosition(mOldPos);
    sgl.dismiss();
}

void CmdSchematicNetPointEdit::performRedo() throw (Exception)
{
    ScopeGuardList sgl;
    mNetPoint.setNetSignal(*mNewNetSignal); // can throw
    sgl.add([&](){mNetPoint.setNetSignal(*mOldNetSignal);});
    mNetPoint.setPinToAttach(mNewSymbolPin); // can throw
    sgl.add([&](){mNetPoint.setPinToAttach(mOldSymbolPin);});
    mNetPoint.setPosition(mNewPos);
    sgl.dismiss();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
