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
#include "cmdcircleedit.h"

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

CmdCircleEdit::CmdCircleEdit(Circle& circle) noexcept
  : UndoCommand(tr("Edit circle")),
    mCircle(circle),
    mOldLayer(&circle.getLayer()),
    mNewLayer(mOldLayer),
    mOldLineWidth(circle.getLineWidth()),
    mNewLineWidth(mOldLineWidth),
    mOldIsFilled(circle.isFilled()),
    mNewIsFilled(mOldIsFilled),
    mOldIsGrabArea(circle.isGrabArea()),
    mNewIsGrabArea(mOldIsGrabArea),
    mOldDiameter(circle.getDiameter()),
    mNewDiameter(mOldDiameter),
    mOldCenter(circle.getCenter()),
    mNewCenter(mOldCenter) {
}

CmdCircleEdit::~CmdCircleEdit() noexcept {
  if (!wasEverExecuted()) {
    performUndo();  // discard possible executed immediate changes
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdCircleEdit::setLayer(const Layer& layer, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewLayer = &layer;
  if (immediate) mCircle.setLayer(*mNewLayer);
}

void CmdCircleEdit::setLineWidth(const UnsignedLength& width,
                                 bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewLineWidth = width;
  if (immediate) mCircle.setLineWidth(mNewLineWidth);
}

void CmdCircleEdit::setIsFilled(bool filled, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewIsFilled = filled;
  if (immediate) mCircle.setIsFilled(mNewIsFilled);
}

void CmdCircleEdit::setIsGrabArea(bool grabArea, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewIsGrabArea = grabArea;
  if (immediate) mCircle.setIsGrabArea(mNewIsGrabArea);
}

void CmdCircleEdit::setDiameter(const PositiveLength& dia,
                                bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDiameter = dia;
  if (immediate) mCircle.setDiameter(mNewDiameter);
}

void CmdCircleEdit::setCenter(const Point& pos, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewCenter = pos;
  if (immediate) mCircle.setCenter(mNewCenter);
}

void CmdCircleEdit::translate(const Point& deltaPos, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewCenter += deltaPos;
  if (immediate) mCircle.setCenter(mNewCenter);
}

void CmdCircleEdit::snapToGrid(const PositiveLength& gridInterval,
                               bool immediate) noexcept {
  setCenter(mNewCenter.mappedToGrid(gridInterval), immediate);
}

void CmdCircleEdit::rotate(const Angle& angle, const Point& center,
                           bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewCenter.rotate(angle, center);
  if (immediate) {
    mCircle.setCenter(mNewCenter);
  }
}

void CmdCircleEdit::mirrorGeometry(Qt::Orientation orientation,
                                   const Point& center,
                                   bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewCenter.mirror(orientation, center);
  if (immediate) {
    mCircle.setCenter(mNewCenter);
  }
}

void CmdCircleEdit::mirrorLayer(bool immediate) noexcept {
  setLayer(mNewLayer->mirrored(), immediate);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdCircleEdit::performExecute() {
  performRedo();  // can throw

  if (mNewLayer != mOldLayer) return true;
  if (mNewLineWidth != mOldLineWidth) return true;
  if (mNewIsFilled != mOldIsFilled) return true;
  if (mNewIsGrabArea != mOldIsGrabArea) return true;
  if (mNewDiameter != mOldDiameter) return true;
  if (mNewCenter != mOldCenter) return true;
  return false;
}

void CmdCircleEdit::performUndo() {
  mCircle.setLayer(*mOldLayer);
  mCircle.setLineWidth(mOldLineWidth);
  mCircle.setIsFilled(mOldIsFilled);
  mCircle.setIsGrabArea(mOldIsGrabArea);
  mCircle.setDiameter(mOldDiameter);
  mCircle.setCenter(mOldCenter);
}

void CmdCircleEdit::performRedo() {
  mCircle.setLayer(*mNewLayer);
  mCircle.setLineWidth(mNewLineWidth);
  mCircle.setIsFilled(mNewIsFilled);
  mCircle.setIsGrabArea(mNewIsGrabArea);
  mCircle.setDiameter(mNewDiameter);
  mCircle.setCenter(mNewCenter);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
