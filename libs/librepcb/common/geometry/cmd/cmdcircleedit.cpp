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
#include "cmdcircleedit.h"
#include "../circle.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdCircleEdit::CmdCircleEdit(Circle& circle) noexcept :
    UndoCommand(tr("Edit circle")), mCircle(circle),
    mOldLayerName(circle.getLayerName()), mNewLayerName(mOldLayerName),
    mOldLineWidth(circle.getLineWidth()), mNewLineWidth(mOldLineWidth),
    mOldIsFilled(circle.isFilled()), mNewIsFilled(mOldIsFilled),
    mOldIsGrabArea(circle.isGrabArea()), mNewIsGrabArea(mOldIsGrabArea),
    mOldRadiusX(circle.getRadiusX()), mNewRadiusX(mOldRadiusX),
    mOldRadiusY(circle.getRadiusY()), mNewRadiusY(mOldRadiusY),
    mOldCenter(circle.getCenter()), mNewCenter(mOldCenter),
    mOldRotation(circle.getRotation()), mNewRotation(mOldRotation)
{
}

CmdCircleEdit::~CmdCircleEdit() noexcept
{
    if (!wasEverExecuted()) {
        performUndo(); // discard possible executed immediate changes
    }
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdCircleEdit::setLayerName(const QString& name, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewLayerName = name;
    if (immediate) mCircle.setLayerName(mNewLayerName);
}

void CmdCircleEdit::setLineWidth(const Length& width, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewLineWidth = width;
    if (immediate) mCircle.setLineWidth(mNewLineWidth);
}

void CmdCircleEdit::setIsFilled(bool filled, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewIsFilled = filled;
    if (immediate) mCircle.setIsFilled(mNewIsFilled);
}

void CmdCircleEdit::setIsGrabArea(bool grabArea, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewIsGrabArea = grabArea;
    if (immediate) mCircle.setIsGrabArea(mNewIsGrabArea);
}

void CmdCircleEdit::setRadiusX(const Length& rx, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewRadiusX = rx;
    if (immediate) mCircle.setRadiusX(mNewRadiusX);
}

void CmdCircleEdit::setRadiusY(const Length& ry, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewRadiusY = ry;
    if (immediate) mCircle.setRadiusY(mNewRadiusY);
}

void CmdCircleEdit::setCenter(const Point& pos, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewCenter = pos;
    if (immediate) mCircle.setCenter(mNewCenter);
}

void CmdCircleEdit::setDeltaToStartCenter(const Point& deltaPos, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewCenter = mOldCenter + deltaPos;
    if (immediate) mCircle.setCenter(mNewCenter);
}

void CmdCircleEdit::setRotation(const Angle& angle, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewRotation = angle;
    if (immediate) mCircle.setRotation(mNewRotation);
}

void CmdCircleEdit::rotate(const Angle& angle, const Point& center, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewCenter.rotate(angle, center);
    mNewRotation += angle;
    if (immediate) {
        mCircle.setCenter(mNewCenter);
        mCircle.setRotation(mNewRotation);
    }
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdCircleEdit::performExecute()
{
    performRedo(); // can throw

    if (mNewLayerName   != mOldLayerName)   return true;
    if (mNewLineWidth   != mOldLineWidth)   return true;
    if (mNewIsFilled    != mOldIsFilled)    return true;
    if (mNewIsGrabArea  != mOldIsGrabArea)  return true;
    if (mNewRadiusX     != mOldRadiusX)     return true;
    if (mNewRadiusY     != mOldRadiusY)     return true;
    if (mNewCenter      != mOldCenter)      return true;
    if (mNewRotation    != mOldRotation)    return true;
    return false;
}

void CmdCircleEdit::performUndo()
{
    mCircle.setLayerName(mOldLayerName);
    mCircle.setLineWidth(mOldLineWidth);
    mCircle.setIsFilled(mOldIsFilled);
    mCircle.setIsGrabArea(mOldIsGrabArea);
    mCircle.setRadiusX(mOldRadiusX);
    mCircle.setRadiusY(mOldRadiusY);
    mCircle.setCenter(mOldCenter);
    mCircle.setRotation(mOldRotation);
}

void CmdCircleEdit::performRedo()
{
    mCircle.setLayerName(mNewLayerName);
    mCircle.setLineWidth(mNewLineWidth);
    mCircle.setIsFilled(mNewIsFilled);
    mCircle.setIsGrabArea(mNewIsGrabArea);
    mCircle.setRadiusX(mNewRadiusX);
    mCircle.setRadiusY(mNewRadiusY);
    mCircle.setCenter(mNewCenter);
    mCircle.setRotation(mNewRotation);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
