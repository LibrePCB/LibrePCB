/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "cmdboardplaneedit.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/types/layer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdBoardPlaneEdit::CmdBoardPlaneEdit(BI_Plane& plane) noexcept
  : UndoCommand(tr("Edit plane")),
    mPlane(plane),
    mOldOutline(plane.getOutline()),
    mNewOutline(mOldOutline),
    mOldLayer(&plane.getLayer()),
    mNewLayer(mOldLayer),
    mOldNetSignal(&plane.getNetSignal()),
    mNewNetSignal(mOldNetSignal),
    mOldMinWidth(plane.getMinWidth()),
    mNewMinWidth(mOldMinWidth),
    mOldMinClearance(plane.getMinClearance()),
    mNewMinClearance(mOldMinClearance),
    mOldConnectStyle(plane.getConnectStyle()),
    mNewConnectStyle(mOldConnectStyle),
    mOldThermalGap(plane.getThermalGap()),
    mNewThermalGap(mOldThermalGap),
    mOldThermalSpokeWidth(plane.getThermalSpokeWidth()),
    mNewThermalSpokeWidth(mOldThermalSpokeWidth),
    mOldPriority(plane.getPriority()),
    mNewPriority(mOldPriority),
    mOldKeepIslands(plane.getKeepIslands()),
    mNewKeepIslands(mOldKeepIslands),
    mOldLocked(plane.isLocked()),
    mNewLocked(mOldLocked) {
}

CmdBoardPlaneEdit::~CmdBoardPlaneEdit() noexcept {
  if (!wasEverExecuted()) {
    mPlane.setOutline(mOldOutline);
    mPlane.setLayer(*mOldLayer);
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdBoardPlaneEdit::translate(const Point& deltaPos,
                                  bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewOutline.translate(deltaPos);
  if (immediate) mPlane.setOutline(mNewOutline);
}

void CmdBoardPlaneEdit::snapToGrid(const PositiveLength& gridInterval,
                                   bool immediate) noexcept {
  setOutline(mNewOutline.mappedToGrid(gridInterval), immediate);
}

void CmdBoardPlaneEdit::rotate(const Angle& angle, const Point& center,
                               bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewOutline.rotate(angle, center);
  if (immediate) mPlane.setOutline(mNewOutline);
}

void CmdBoardPlaneEdit::mirror(const Point& center, Qt::Orientation orientation,
                               bool immediate) noexcept {
  setLayer(mNewLayer->mirrored(), immediate);
  setOutline(mNewOutline.mirrored(orientation, center), immediate);
}

void CmdBoardPlaneEdit::setOutline(const Path& outline,
                                   bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewOutline = outline;
  if (immediate) mPlane.setOutline(mNewOutline);
}

void CmdBoardPlaneEdit::setLayer(const Layer& layer, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewLayer = &layer;
  if (immediate) mPlane.setLayer(*mNewLayer);
}

void CmdBoardPlaneEdit::setNetSignal(NetSignal& netsignal) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewNetSignal = &netsignal;
}

void CmdBoardPlaneEdit::setMinWidth(const UnsignedLength& minWidth) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewMinWidth = minWidth;
}

void CmdBoardPlaneEdit::setMinClearance(
    const UnsignedLength& minClearance) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewMinClearance = minClearance;
}

void CmdBoardPlaneEdit::setConnectStyle(BI_Plane::ConnectStyle style) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewConnectStyle = style;
}

void CmdBoardPlaneEdit::setThermalGap(const PositiveLength& gap) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewThermalGap = gap;
}
void CmdBoardPlaneEdit::setThermalSpokeWidth(
    const PositiveLength& width) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewThermalSpokeWidth = width;
}

void CmdBoardPlaneEdit::setPriority(int priority) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPriority = priority;
}

void CmdBoardPlaneEdit::setKeepIslands(bool keep) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewKeepIslands = keep;
}

void CmdBoardPlaneEdit::setLocked(bool locked) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewLocked = locked;
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardPlaneEdit::performExecute() {
  performRedo();  // can throw

  if (mNewOutline != mOldOutline) return true;
  if (mNewLayer != mOldLayer) return true;
  if (mNewNetSignal != mOldNetSignal) return true;
  if (mNewMinWidth != mOldMinWidth) return true;
  if (mNewMinClearance != mOldMinClearance) return true;
  if (mNewConnectStyle != mOldConnectStyle) return true;
  if (mNewThermalGap != mOldThermalGap) return true;
  if (mNewThermalSpokeWidth != mOldThermalSpokeWidth) return true;
  if (mNewPriority != mOldPriority) return true;
  if (mNewKeepIslands != mOldKeepIslands) return true;
  if (mNewLocked != mOldLocked) return true;
  return false;
}

void CmdBoardPlaneEdit::performUndo() {
  mPlane.setNetSignal(*mOldNetSignal);  // can throw
  mPlane.setOutline(mOldOutline);
  mPlane.setLayer(*mOldLayer);
  mPlane.setMinWidth(mOldMinWidth);
  mPlane.setMinClearance(mOldMinClearance);
  mPlane.setConnectStyle(mOldConnectStyle);
  mPlane.setThermalGap(mOldThermalGap);
  mPlane.setThermalSpokeWidth(mOldThermalSpokeWidth);
  mPlane.setPriority(mOldPriority);
  mPlane.setKeepIslands(mOldKeepIslands);
  mPlane.setLocked(mOldLocked);
}

void CmdBoardPlaneEdit::performRedo() {
  mPlane.setNetSignal(*mNewNetSignal);  // can throw
  mPlane.setOutline(mNewOutline);
  mPlane.setLayer(*mNewLayer);
  mPlane.setMinWidth(mNewMinWidth);
  mPlane.setMinClearance(mNewMinClearance);
  mPlane.setConnectStyle(mNewConnectStyle);
  mPlane.setThermalGap(mNewThermalGap);
  mPlane.setThermalSpokeWidth(mNewThermalSpokeWidth);
  mPlane.setPriority(mNewPriority);
  mPlane.setKeepIslands(mNewKeepIslands);
  mPlane.setLocked(mNewLocked);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
