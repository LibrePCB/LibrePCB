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
#include "cmdpolygonsegmentedit.h"
#include "../polygon.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdPolygonSegmentEdit::CmdPolygonSegmentEdit(PolygonSegment& segment) noexcept :
    UndoCommand(tr("Edit polygon segment")), mSegment(segment),
    mOldEndPos(segment.getEndPos()), mNewEndPos(mOldEndPos),
    mOldAngle(segment.getAngle()), mNewAngle(mOldAngle)
{
}

CmdPolygonSegmentEdit::~CmdPolygonSegmentEdit() noexcept
{
    if (!wasEverExecuted()) {
        performUndo(); // discard possible executed immediate changes
    }
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdPolygonSegmentEdit::setEndPos(const Point& pos, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewEndPos = pos;
    if (immediate) mSegment.setEndPos(mNewEndPos);
}

void CmdPolygonSegmentEdit::setDeltaToStartPos(const Point& deltaPos, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewEndPos = mOldEndPos + deltaPos;
    if (immediate) mSegment.setEndPos(mNewEndPos);
}

void CmdPolygonSegmentEdit::rotate(const Angle& angle, const Point& center, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewEndPos.rotate(angle, center);
    if (immediate) mSegment.setEndPos(mNewEndPos);
}

void CmdPolygonSegmentEdit::setAngle(const Angle& angle, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewAngle = angle;
    if (immediate) mSegment.setAngle(mNewAngle);
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdPolygonSegmentEdit::performExecute()
{
    performRedo(); // can throw

    if (mNewEndPos  != mOldEndPos)  return true;
    if (mNewAngle   != mOldAngle)   return true;
    return false;
}

void CmdPolygonSegmentEdit::performUndo()
{
    mSegment.setEndPos(mOldEndPos);
    mSegment.setAngle(mOldAngle);
}

void CmdPolygonSegmentEdit::performRedo()
{
    mSegment.setEndPos(mNewEndPos);
    mSegment.setAngle(mNewAngle);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
