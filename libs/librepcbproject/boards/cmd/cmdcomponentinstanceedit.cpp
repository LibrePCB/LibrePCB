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
#include "cmdcomponentinstanceedit.h"
#include "../componentinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdComponentInstanceEdit::CmdComponentInstanceEdit(ComponentInstance& cmp, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Edit component instance"), parent), mComponent(cmp),
    mOldPos(mComponent.getPosition()), mNewPos(mOldPos),
    mOldRotation(mComponent.getRotation()), mNewRotation(mOldRotation),
    mOldMirrored(mComponent.getIsMirrored()), mNewMirrored(mOldMirrored)
{
}

CmdComponentInstanceEdit::~CmdComponentInstanceEdit() noexcept
{
    if ((mRedoCount == 0) && (mUndoCount == 0))
    {
        mComponent.setPosition(mOldPos);
        mComponent.setRotation(mOldRotation);
        mComponent.setIsMirrored(mOldMirrored);
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void CmdComponentInstanceEdit::setPosition(Point& pos, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewPos = pos;
    if (immediate) mComponent.setPosition(mNewPos);
}

void CmdComponentInstanceEdit::setDeltaToStartPos(Point& deltaPos, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewPos = mOldPos + deltaPos;
    if (immediate) mComponent.setPosition(mNewPos);
}

void CmdComponentInstanceEdit::setRotation(const Angle& angle, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewRotation = angle;
    if (immediate) mComponent.setRotation(mNewRotation);
}

void CmdComponentInstanceEdit::rotate(const Angle& angle, const Point& center, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewPos.rotate(angle, center);
    mNewRotation += angle;
    if (immediate)
    {
        mComponent.setPosition(mNewPos);
        mComponent.setRotation(mNewRotation);
    }
}

void CmdComponentInstanceEdit::setMirrored(bool mirrored, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewMirrored = mirrored;
    if (immediate) mComponent.setIsMirrored(mNewMirrored);
}

void CmdComponentInstanceEdit::mirror(const Point& center, bool vertical, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewMirrored = !mNewMirrored;
    if (vertical)
    {
        mNewPos.setY(mNewPos.getY() + Length(2) * (center.getY() - mNewPos.getY()));
        mNewRotation += Angle::deg180();
    }
    else
    {
        mNewPos.setX(mNewPos.getX() + Length(2) * (center.getX() - mNewPos.getX()));
    }
    if (immediate)
    {
        mComponent.setIsMirrored(mNewMirrored);
        mComponent.setPosition(mNewPos);
        mComponent.setRotation(mNewRotation);
    }
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdComponentInstanceEdit::redo() throw (Exception)
{
    try
    {
        mComponent.setPosition(mNewPos);
        mComponent.setRotation(mNewRotation);
        mComponent.setIsMirrored(mNewMirrored);
        UndoCommand::redo();
    }
    catch (Exception &e)
    {
        mComponent.setPosition(mOldPos);
        mComponent.setRotation(mOldRotation);
        mComponent.setIsMirrored(mOldMirrored);
        throw;
    }
}

void CmdComponentInstanceEdit::undo() throw (Exception)
{
    try
    {
        mComponent.setPosition(mOldPos);
        mComponent.setRotation(mOldRotation);
        mComponent.setIsMirrored(mOldMirrored);
        UndoCommand::undo();
    }
    catch (Exception &e)
    {
        mComponent.setPosition(mNewPos);
        mComponent.setRotation(mNewRotation);
        mComponent.setIsMirrored(mNewMirrored);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
