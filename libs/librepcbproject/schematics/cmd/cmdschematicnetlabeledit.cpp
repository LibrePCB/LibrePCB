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
#include "cmdschematicnetlabeledit.h"
#include "../items/si_netlabel.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdSchematicNetLabelEdit::CmdSchematicNetLabelEdit(SI_NetLabel& netlabel,
                                                   UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Edit netlabel"), parent), mNetLabel(netlabel),
    mOldNetSignal(&netlabel.getNetSignal()), mNewNetSignal(mOldNetSignal),
    mOldPos(netlabel.getPosition()), mNewPos(mOldPos),
    mOldRotation(netlabel.getAngle()), mNewRotation(mOldRotation)
{
}

CmdSchematicNetLabelEdit::~CmdSchematicNetLabelEdit() noexcept
{
    if ((mRedoCount == 0) && (mUndoCount == 0))
    {
        // revert temporary changes
        mNetLabel.setNetSignal(*mOldNetSignal);
        mNetLabel.setPosition(mOldPos);
        mNetLabel.setAngle(mOldRotation);
    }
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdSchematicNetLabelEdit::setNetSignal(NetSignal& netsignal, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewNetSignal = &netsignal;
    if (immediate) mNetLabel.setNetSignal(*mNewNetSignal);
}

void CmdSchematicNetLabelEdit::setPosition(const Point& position, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewPos = position;
    if (immediate) mNetLabel.setPosition(mNewPos);
}

void CmdSchematicNetLabelEdit::setDeltaToStartPos(const Point& deltaPos, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewPos = mOldPos + deltaPos;
    if (immediate) mNetLabel.setPosition(mNewPos);
}

void CmdSchematicNetLabelEdit::setRotation(const Angle& angle, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewRotation = angle;
    if (immediate) mNetLabel.setAngle(mNewRotation);
}

void CmdSchematicNetLabelEdit::rotate(const Angle& angle, const Point& center, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewPos.rotate(angle, center);
    mNewRotation += angle;
    if (immediate)
    {
        mNetLabel.setPosition(mNewPos);
        mNetLabel.setAngle(mNewRotation);
    }
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdSchematicNetLabelEdit::redo() throw (Exception)
{
    try
    {
        mNetLabel.setNetSignal(*mNewNetSignal);
        mNetLabel.setPosition(mNewPos);
        mNetLabel.setAngle(mNewRotation);
        UndoCommand::redo();
    }
    catch (Exception& e)
    {
        mNetLabel.setNetSignal(*mOldNetSignal);
        mNetLabel.setPosition(mOldPos);
        mNetLabel.setAngle(mOldRotation);
        throw;
    }
}

void CmdSchematicNetLabelEdit::undo() throw (Exception)
{
    try
    {
        mNetLabel.setNetSignal(*mOldNetSignal);
        mNetLabel.setPosition(mOldPos);
        mNetLabel.setAngle(mOldRotation);
        UndoCommand::undo();
    }
    catch (Exception& e)
    {
        mNetLabel.setNetSignal(*mNewNetSignal);
        mNetLabel.setPosition(mNewPos);
        mNetLabel.setAngle(mNewRotation);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
