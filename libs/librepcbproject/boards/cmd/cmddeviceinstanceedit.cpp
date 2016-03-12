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
#include "../items/bi_device.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdDeviceInstanceEdit::CmdDeviceInstanceEdit(BI_Device& dev) noexcept :
    UndoCommand(tr("Edit device instance")), mDevice(dev),
    mOldPos(mDevice.getPosition()), mNewPos(mOldPos),
    mOldRotation(mDevice.getRotation()), mNewRotation(mOldRotation),
    mOldMirrored(mDevice.getIsMirrored()), mNewMirrored(mOldMirrored)
{
}

CmdDeviceInstanceEdit::~CmdDeviceInstanceEdit() noexcept
{
    if (!wasEverExecuted()) {
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
    Q_ASSERT(!wasEverExecuted());
    mNewPos = pos;
    if (immediate) mDevice.setPosition(mNewPos);
}

void CmdDeviceInstanceEdit::setDeltaToStartPos(Point& deltaPos, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewPos = mOldPos + deltaPos;
    if (immediate) mDevice.setPosition(mNewPos);
}

void CmdDeviceInstanceEdit::setRotation(const Angle& angle, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewRotation = angle;
    if (immediate) mDevice.setRotation(mNewRotation);
}

void CmdDeviceInstanceEdit::rotate(const Angle& angle, const Point& center, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewPos.rotate(angle, center);
    mNewRotation += mNewMirrored ? -angle : angle; // mirror --> rotation direction is inverted!
    if (immediate)
    {
        mDevice.setPosition(mNewPos);
        mDevice.setRotation(mNewRotation);
    }
}

void CmdDeviceInstanceEdit::setMirrored(bool mirrored, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewMirrored = mirrored;
    if (immediate) mDevice.setIsMirrored(mNewMirrored);
}

void CmdDeviceInstanceEdit::mirror(const Point& center, Qt::Orientation orientation,
                                   bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewMirrored = !mNewMirrored;
    switch (orientation)
    {
        case Qt::Vertical: {
            mNewPos.setY(mNewPos.getY() + Length(2) * (center.getY() - mNewPos.getY()));
            mNewRotation += Angle::deg180();
            break;
        }
        case Qt::Horizontal: {
            mNewPos.setX(mNewPos.getX() + Length(2) * (center.getX() - mNewPos.getX()));
            break;
        }
        default: {
            qCritical() << "Invalid orientation:" << orientation;
            break;
        }
    }
    if (immediate) {
        mDevice.setIsMirrored(mNewMirrored);
        mDevice.setPosition(mNewPos);
        mDevice.setRotation(mNewRotation);
    }
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdDeviceInstanceEdit::performExecute() throw (Exception)
{
    performRedo(); // can throw

    return true; // TODO: determine if the device was really modified
}

void CmdDeviceInstanceEdit::performUndo() throw (Exception)
{
    mDevice.setPosition(mOldPos);
    mDevice.setRotation(mOldRotation);
    mDevice.setIsMirrored(mOldMirrored);
}

void CmdDeviceInstanceEdit::performRedo() throw (Exception)
{
    mDevice.setPosition(mNewPos);
    mDevice.setRotation(mNewRotation);
    mDevice.setIsMirrored(mNewMirrored);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
