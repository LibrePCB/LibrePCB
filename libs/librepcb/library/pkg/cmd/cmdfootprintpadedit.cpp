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
#include "cmdfootprintpadedit.h"

#include "../footprintpad.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdFootprintPadEdit::CmdFootprintPadEdit(FootprintPad& pad) noexcept
  : UndoCommand(tr("Edit footprint pad")),
    mPad(pad),
    mOldPackagePadUuid(pad.getPackagePadUuid()),
    mNewPackagePadUuid(mOldPackagePadUuid),
    mOldBoardSide(pad.getBoardSide()),
    mNewBoardSide(mOldBoardSide),
    mOldShape(pad.getShape()),
    mNewShape(mOldShape),
    mOldWidth(pad.getWidth()),
    mNewWidth(mOldWidth),
    mOldHeight(pad.getHeight()),
    mNewHeight(mOldHeight),
    mOldPos(pad.getPosition()),
    mNewPos(mOldPos),
    mOldRotation(pad.getRotation()),
    mNewRotation(mOldRotation),
    mOldDrillSize(pad.getDrillSize()),
    mNewDrillSize(mOldDrillSize) {
}

CmdFootprintPadEdit::~CmdFootprintPadEdit() noexcept {
  if (!wasEverExecuted()) {
    try {
      performUndo();
    } catch (...) {
      qCritical() << "Undo failed!";
    }
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdFootprintPadEdit::setPackagePadUuid(const Uuid& pad,
                                            bool        immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPackagePadUuid = pad;
  if (immediate) mPad.setPackagePadUuid(mNewPackagePadUuid);
}

void CmdFootprintPadEdit::setBoardSide(FootprintPad::BoardSide side,
                                       bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewBoardSide = side;
  if (immediate) mPad.setBoardSide(mNewBoardSide);
}

void CmdFootprintPadEdit::setShape(FootprintPad::Shape shape,
                                   bool                immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewShape = shape;
  if (immediate) mPad.setShape(mNewShape);
}

void CmdFootprintPadEdit::setWidth(const PositiveLength& width,
                                   bool                  immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewWidth = width;
  if (immediate) mPad.setWidth(mNewWidth);
}

void CmdFootprintPadEdit::setHeight(const PositiveLength& height,
                                    bool                  immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewHeight = height;
  if (immediate) mPad.setHeight(mNewHeight);
}

void CmdFootprintPadEdit::setDrillSize(const tl::optional<DrillSize>& drillSize,
                                       bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewDrillSize = drillSize;
  if (immediate) mPad.setDrillSize(mNewDrillSize);
}

void CmdFootprintPadEdit::setPosition(const Point& pos,
                                      bool         immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos = pos;
  if (immediate) mPad.setPosition(mNewPos);
}

void CmdFootprintPadEdit::translate(const Point& deltaPos,
                                    bool         immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos += deltaPos;
  if (immediate) mPad.setPosition(mNewPos);
}

void CmdFootprintPadEdit::setRotation(const Angle& angle,
                                      bool         immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewRotation = angle;
  if (immediate) mPad.setRotation(mNewRotation);
}

void CmdFootprintPadEdit::rotate(const Angle& angle, const Point& center,
                                 bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos.rotate(angle, center);
  mNewRotation += angle;
  if (immediate) {
    mPad.setPosition(mNewPos);
    mPad.setRotation(mNewRotation);
  }
}

void CmdFootprintPadEdit::mirrorGeometry(Qt::Orientation orientation,
                                         const Point&    center,
                                         bool            immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewPos.mirror(orientation, center);
  if (orientation == Qt::Horizontal) {
    mNewRotation = Angle::deg180() - mNewRotation;
  } else {
    mNewRotation = -mNewRotation;
  }
  if (immediate) {
    mPad.setPosition(mNewPos);
    mPad.setRotation(mNewRotation);
  }
}

void CmdFootprintPadEdit::mirrorLayer(bool immediate) noexcept {
  switch (mNewBoardSide) {
    case FootprintPad::BoardSide::BOTTOM:
      mNewBoardSide = FootprintPad::BoardSide::TOP;
      break;
    case FootprintPad::BoardSide::TOP:
      mNewBoardSide = FootprintPad::BoardSide::BOTTOM;
      break;
    default:
      break;
  }
  if (immediate) {
    mPad.setBoardSide(mNewBoardSide);
  }
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdFootprintPadEdit::performExecute() {
  performRedo();  // can throw

  if (mNewPackagePadUuid != mOldPackagePadUuid) return true;
  if (mNewBoardSide != mOldBoardSide) return true;
  if (mNewShape != mOldShape) return true;
  if (mNewWidth != mOldWidth) return true;
  if (mNewHeight != mOldHeight) return true;
  if (mNewPos != mOldPos) return true;
  if (mNewRotation != mOldRotation) return true;
  if (mNewDrillSize != mOldDrillSize) return true;
  return false;
}

void CmdFootprintPadEdit::performUndo() {
  mPad.setPackagePadUuid(mOldPackagePadUuid);
  mPad.setBoardSide(mOldBoardSide);
  mPad.setShape(mOldShape);
  mPad.setWidth(mOldWidth);
  mPad.setHeight(mOldHeight);
  mPad.setPosition(mOldPos);
  mPad.setRotation(mOldRotation);
  mPad.setDrillSize(mOldDrillSize);
}

void CmdFootprintPadEdit::performRedo() {
  mPad.setPackagePadUuid(mNewPackagePadUuid);
  mPad.setBoardSide(mNewBoardSide);
  mPad.setShape(mNewShape);
  mPad.setWidth(mNewWidth);
  mPad.setHeight(mNewHeight);
  mPad.setPosition(mNewPos);
  mPad.setRotation(mNewRotation);
  mPad.setDrillSize(mNewDrillSize);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
