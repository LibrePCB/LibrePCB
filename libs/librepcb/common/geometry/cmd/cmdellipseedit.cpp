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
#include "cmdellipseedit.h"
#include "../ellipse.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdEllipseEdit::CmdEllipseEdit(Ellipse& ellipse) noexcept :
    UndoCommand(tr("Edit ellipse")), mEllipse(ellipse),
    mOldLayerName(ellipse.getLayerName()), mNewLayerName(mOldLayerName),
    mOldLineWidth(ellipse.getLineWidth()), mNewLineWidth(mOldLineWidth),
    mOldIsFilled(ellipse.isFilled()), mNewIsFilled(mOldIsFilled),
    mOldIsGrabArea(ellipse.isGrabArea()), mNewIsGrabArea(mOldIsGrabArea),
    mOldRadiusX(ellipse.getRadiusX()), mNewRadiusX(mOldRadiusX),
    mOldRadiusY(ellipse.getRadiusY()), mNewRadiusY(mOldRadiusY),
    mOldCenter(ellipse.getCenter()), mNewCenter(mOldCenter),
    mOldRotation(ellipse.getRotation()), mNewRotation(mOldRotation)
{
}

CmdEllipseEdit::~CmdEllipseEdit() noexcept
{
    if (!wasEverExecuted()) {
        performUndo(); // discard possible executed immediate changes
    }
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdEllipseEdit::setLayerName(const QString& name, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewLayerName = name;
    if (immediate) mEllipse.setLayerName(mNewLayerName);
}

void CmdEllipseEdit::setLineWidth(const Length& width, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewLineWidth = width;
    if (immediate) mEllipse.setLineWidth(mNewLineWidth);
}

void CmdEllipseEdit::setIsFilled(bool filled, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewIsFilled = filled;
    if (immediate) mEllipse.setIsFilled(mNewIsFilled);
}

void CmdEllipseEdit::setIsGrabArea(bool grabArea, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewIsGrabArea = grabArea;
    if (immediate) mEllipse.setIsGrabArea(mNewIsGrabArea);
}

void CmdEllipseEdit::setRadiusX(const Length& rx, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewRadiusX = rx;
    if (immediate) mEllipse.setRadiusX(mNewRadiusX);
}

void CmdEllipseEdit::setRadiusY(const Length& ry, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewRadiusY = ry;
    if (immediate) mEllipse.setRadiusY(mNewRadiusY);
}

void CmdEllipseEdit::setCenter(const Point& pos, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewCenter = pos;
    if (immediate) mEllipse.setCenter(mNewCenter);
}

void CmdEllipseEdit::setDeltaToStartCenter(const Point& deltaPos, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewCenter = mOldCenter + deltaPos;
    if (immediate) mEllipse.setCenter(mNewCenter);
}

void CmdEllipseEdit::setRotation(const Angle& angle, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewRotation = angle;
    if (immediate) mEllipse.setRotation(mNewRotation);
}

void CmdEllipseEdit::rotate(const Angle& angle, const Point& center, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewCenter.rotate(angle, center);
    mNewRotation += angle;
    if (immediate) {
        mEllipse.setCenter(mNewCenter);
        mEllipse.setRotation(mNewRotation);
    }
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdEllipseEdit::performExecute()
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

void CmdEllipseEdit::performUndo()
{
    mEllipse.setLayerName(mOldLayerName);
    mEllipse.setLineWidth(mOldLineWidth);
    mEllipse.setIsFilled(mOldIsFilled);
    mEllipse.setIsGrabArea(mOldIsGrabArea);
    mEllipse.setRadiusX(mOldRadiusX);
    mEllipse.setRadiusY(mOldRadiusY);
    mEllipse.setCenter(mOldCenter);
    mEllipse.setRotation(mOldRotation);
}

void CmdEllipseEdit::performRedo()
{
    mEllipse.setLayerName(mNewLayerName);
    mEllipse.setLineWidth(mNewLineWidth);
    mEllipse.setIsFilled(mNewIsFilled);
    mEllipse.setIsGrabArea(mNewIsGrabArea);
    mEllipse.setRadiusX(mNewRadiusX);
    mEllipse.setRadiusY(mNewRadiusY);
    mEllipse.setCenter(mNewCenter);
    mEllipse.setRotation(mNewRotation);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
