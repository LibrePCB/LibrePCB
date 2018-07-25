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
#include "cmdboardplaneedit.h"
#include "../board.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdBoardPlaneEdit::CmdBoardPlaneEdit(BI_Plane& plane, bool rebuildOnChanges) noexcept :
    UndoCommand(tr("Edit plane")), mPlane(plane), mDoRebuildOnChanges(rebuildOnChanges),
    mOldOutline(plane.getOutline()), mNewOutline(mOldOutline),
    mOldLayerName(plane.getLayerName()), mNewLayerName(mOldLayerName),
    mOldNetSignal(&plane.getNetSignal()), mNewNetSignal(mOldNetSignal),
    mOldMinWidth(plane.getMinWidth()), mNewMinWidth(mOldMinWidth),
    mOldMinClearance(plane.getMinClearance()), mNewMinClearance(mOldMinClearance),
    mOldConnectStyle(plane.getConnectStyle()), mNewConnectStyle(mOldConnectStyle),
    mOldPriority(plane.getPriority()), mNewPriority(mOldPriority),
    mOldKeepOrphans(plane.getKeepOrphans()), mNewKeepOrphans(mOldKeepOrphans)
{
}

CmdBoardPlaneEdit::~CmdBoardPlaneEdit() noexcept
{
    if (!wasEverExecuted()) {
        mPlane.setOutline(mOldOutline);
        mPlane.setLayerName(mOldLayerName);
    }
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdBoardPlaneEdit::setDeltaToStartPos(const Point& deltaPos, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewOutline = mOldOutline.translated(deltaPos);
    if (immediate) mPlane.setOutline(mNewOutline);
}

void CmdBoardPlaneEdit::rotate(const Angle& angle, const Point& center, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewOutline = mOldOutline.rotated(angle, center);
    if (immediate) mPlane.setOutline(mNewOutline);
}

void CmdBoardPlaneEdit::setOutline(const Path& outline, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewOutline = outline;
    if (immediate) mPlane.setOutline(mNewOutline);
}

void CmdBoardPlaneEdit::setLayerName(const QString& layerName, bool immediate) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewLayerName = layerName;
    if (immediate) mPlane.setLayerName(mNewLayerName);
}

void CmdBoardPlaneEdit::setNetSignal(NetSignal& netsignal) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewNetSignal = &netsignal;
}

void CmdBoardPlaneEdit::setMinWidth(const UnsignedLength& minWidth) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewMinWidth = minWidth;
}

void CmdBoardPlaneEdit::setMinClearance(const UnsignedLength& minClearance) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewMinClearance = minClearance;
}

void CmdBoardPlaneEdit::setConnectStyle(BI_Plane::ConnectStyle style) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewConnectStyle = style;
}

void CmdBoardPlaneEdit::setPriority(int priority) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewPriority = priority;
}

void CmdBoardPlaneEdit::setKeepOrphans(bool keepOrphans) noexcept
{
    Q_ASSERT(!wasEverExecuted());
    mNewKeepOrphans = keepOrphans;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

bool CmdBoardPlaneEdit::performExecute()
{
    performRedo(); // can throw

    if (mNewOutline != mOldOutline)             return true;
    if (mNewLayerName != mOldLayerName)         return true;
    if (mNewNetSignal != mOldNetSignal)         return true;
    if (mNewMinWidth != mOldMinWidth)           return true;
    if (mNewMinClearance != mOldMinClearance)   return true;
    if (mNewConnectStyle != mOldConnectStyle)   return true;
    if (mNewPriority != mOldPriority)           return true;
    if (mNewKeepOrphans != mOldKeepOrphans)     return true;
    return false;
}

void CmdBoardPlaneEdit::performUndo()
{
    mPlane.setNetSignal(*mOldNetSignal); // can throw
    mPlane.setOutline(mOldOutline);
    mPlane.setLayerName(mOldLayerName);
    mPlane.setMinWidth(mOldMinWidth);
    mPlane.setMinClearance(mOldMinClearance);
    mPlane.setConnectStyle(mOldConnectStyle);
    mPlane.setPriority(mOldPriority);
    mPlane.setKeepOrphans(mOldKeepOrphans);

    // rebuild all planes to see the changes
    if (mDoRebuildOnChanges) mPlane.getBoard().rebuildAllPlanes();
}

void CmdBoardPlaneEdit::performRedo()
{
    mPlane.setNetSignal(*mNewNetSignal); // can throw
    mPlane.setOutline(mNewOutline);
    mPlane.setLayerName(mNewLayerName);
    mPlane.setMinWidth(mNewMinWidth);
    mPlane.setMinClearance(mNewMinClearance);
    mPlane.setConnectStyle(mNewConnectStyle);
    mPlane.setPriority(mNewPriority);
    mPlane.setKeepOrphans(mNewKeepOrphans);

    // rebuild all planes to see the changes
    if (mDoRebuildOnChanges) mPlane.getBoard().rebuildAllPlanes();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
