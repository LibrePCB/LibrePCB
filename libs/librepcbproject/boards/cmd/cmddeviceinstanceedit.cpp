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
#include "cmddeviceinstanceedit.h"
#include "../deviceinstance.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdDeviceInstanceEdit::CmdDeviceInstanceEdit(DeviceInstance& dev, UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Edit device instance"), parent), mDevice(dev),
    mOldPos(mDevice.getPosition()), mNewPos(mOldPos),
    mOldRotation(mDevice.getRotation()), mNewRotation(mOldRotation),
    mOldMirrored(mDevice.getIsMirrored()), mNewMirrored(mOldMirrored)
{
}

CmdDeviceInstanceEdit::~CmdDeviceInstanceEdit() noexcept
{
    if ((mRedoCount == 0) && (mUndoCount == 0))
    {
        mDevice.setPosition(mOldPos);
        mDevice.setRotation(mOldRotation);
        mDevice.setIsMirrored(mOldMirrored);
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void CmdDeviceInstanceEdit::setPosition(Point& pos, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewPos = pos;
    if (immediate) mDevice.setPosition(mNewPos);
}

void CmdDeviceInstanceEdit::setDeltaToStartPos(Point& deltaPos, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewPos = mOldPos + deltaPos;
    if (immediate) mDevice.setPosition(mNewPos);
}

void CmdDeviceInstanceEdit::setRotation(const Angle& angle, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewRotation = angle;
    if (immediate) mDevice.setRotation(mNewRotation);
}

void CmdDeviceInstanceEdit::rotate(const Angle& angle, const Point& center, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewPos.rotate(angle, center);
    mNewRotation += angle;
    if (immediate)
    {
        mDevice.setPosition(mNewPos);
        mDevice.setRotation(mNewRotation);
    }
}

void CmdDeviceInstanceEdit::setMirrored(bool mirrored, bool immediate) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNewMirrored = mirrored;
    if (immediate) mDevice.setIsMirrored(mNewMirrored);
}

void CmdDeviceInstanceEdit::mirror(const Point& center, bool vertical, bool immediate) noexcept
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
        mDevice.setIsMirrored(mNewMirrored);
        mDevice.setPosition(mNewPos);
        mDevice.setRotation(mNewRotation);
    }
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdDeviceInstanceEdit::redo() throw (Exception)
{
    try
    {
        mDevice.setPosition(mNewPos);
        mDevice.setRotation(mNewRotation);
        mDevice.setIsMirrored(mNewMirrored);
        UndoCommand::redo();
    }
    catch (Exception &e)
    {
        mDevice.setPosition(mOldPos);
        mDevice.setRotation(mOldRotation);
        mDevice.setIsMirrored(mOldMirrored);
        throw;
    }
}

void CmdDeviceInstanceEdit::undo() throw (Exception)
{
    try
    {
        mDevice.setPosition(mOldPos);
        mDevice.setRotation(mOldRotation);
        mDevice.setIsMirrored(mOldMirrored);
        UndoCommand::undo();
    }
    catch (Exception &e)
    {
        mDevice.setPosition(mNewPos);
        mDevice.setRotation(mNewRotation);
        mDevice.setIsMirrored(mNewMirrored);
        throw;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
