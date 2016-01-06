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
#include "cmdschematicnetpointedit.h"
#include "../items/si_netpoint.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSchematicNetPointEdit::CmdSchematicNetPointEdit(SI_NetPoint& point,
                                                   UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Edit netpoint"), parent), mNetPoint(point),
    mOldNetSignal(point.getNetSignal()), mNewNetSignal(mOldNetSignal),
    mOldPos(point.getPosition()), mNewPos(mOldPos)
{
}

CmdSchematicNetPointEdit::~CmdSchematicNetPointEdit() noexcept
{
    if ((mRedoCount == 0) && (mUndoCount == 0))
    {
        mNetPoint.setPosition(mOldPos);
    }
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdSchematicNetPointEdit::setNetSignal(NetSignal& netsignal) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewNetSignal = &netsignal;
}

void CmdSchematicNetPointEdit::setPosition(const Point& pos, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewPos = pos;
    if (immediate) mNetPoint.setPosition(mNewPos);
}

void CmdSchematicNetPointEdit::setDeltaToStartPos(const Point& deltaPos, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewPos = mOldPos + deltaPos;
    if (immediate) mNetPoint.setPosition(mNewPos);
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSchematicNetPointEdit::redo() throw (Exception)
{
    try
    {
        mNetPoint.setNetSignal(*mNewNetSignal);
        mNetPoint.setPosition(mNewPos);
        UndoCommand::redo();
    }
    catch (Exception &e)
    {
        mNetPoint.setNetSignal(*mOldNetSignal);
        mNetPoint.setPosition(mOldPos);
        throw;
    }
}

void CmdSchematicNetPointEdit::undo() throw (Exception)
{
    try
    {
        mNetPoint.setNetSignal(*mOldNetSignal);
        mNetPoint.setPosition(mOldPos);
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mNetPoint.setNetSignal(*mNewNetSignal);
        mNetPoint.setPosition(mNewPos);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
