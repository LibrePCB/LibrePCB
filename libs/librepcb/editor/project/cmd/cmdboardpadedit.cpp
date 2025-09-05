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
#include "cmdboardpadedit.h"

#include <librepcb/core/project/board/items/bi_pad.h>
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

CmdBoardPadEdit::CmdBoardPadEdit(BI_Pad& pad) noexcept
  : UndoCommand(tr("Edit Pad")),
    mPad(pad),
    mOldProperties(pad.getProperties()),
    mNewProperties(mOldProperties) {
  Q_ASSERT(pad.getNetSegment());  // Only board pads are mutable.
}

CmdBoardPadEdit::~CmdBoardPadEdit() noexcept {
  if (!wasEverExecuted()) {
    try {
      performUndo();
    } catch (...) {
      qCritical() << "Undo failed in CmdBoardPadEdit destructor!";
    }
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdBoardPadEdit::setComponentSideAndHoles(Pad::ComponentSide side,
                                               const PadHoleList& holes,
                                               bool immediate) {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setComponentSide(side);
  mNewProperties.getHoles() = holes;
  if (immediate) mPad.setComponentSideAndHoles(side, holes);  // can throw
}

void CmdBoardPadEdit::setFunction(Pad::Function function,
                                  bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setFunction(function);
  if (immediate) mPad.setFunction(function);
}

void CmdBoardPadEdit::setShape(Pad::Shape shape, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setShape(shape);
  if (immediate) mPad.setShape(shape);
}

void CmdBoardPadEdit::setWidth(const PositiveLength& width,
                               bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setWidth(width);
  if (immediate) mPad.setWidth(width);
}

void CmdBoardPadEdit::setHeight(const PositiveLength& height,
                                bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setHeight(height);
  if (immediate) mPad.setHeight(height);
}

void CmdBoardPadEdit::setRadius(const UnsignedLimitedRatio& radius,
                                bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setRadius(radius);
  if (immediate) mPad.setRadius(radius);
}

void CmdBoardPadEdit::setCustomShapeOutline(const Path& outline) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setCustomShapeOutline(outline);
}

void CmdBoardPadEdit::setStopMaskConfig(const MaskConfig& config,
                                        bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setStopMaskConfig(config);
  if (immediate) mPad.setStopMaskConfig(config);
}

void CmdBoardPadEdit::setSolderPasteConfig(const MaskConfig& config) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setSolderPasteConfig(config);
}

void CmdBoardPadEdit::setCopperClearance(const UnsignedLength& clearance,
                                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setCopperClearance(clearance);
  if (immediate) mPad.setCopperClearance(clearance);
}

void CmdBoardPadEdit::setPosition(const Point& pos, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setPosition(pos);
  if (immediate) mPad.setPosition(pos);
}

void CmdBoardPadEdit::translate(const Point& deltaPos,
                                bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setPosition(mNewProperties.getPosition() + deltaPos);
  if (immediate) mPad.setPosition(mNewProperties.getPosition());
}

void CmdBoardPadEdit::snapToGrid(const PositiveLength& gridInterval,
                                 bool immediate) noexcept {
  setPosition(mNewProperties.getPosition().mappedToGrid(gridInterval),
              immediate);
}

void CmdBoardPadEdit::setRotation(const Angle& angle, bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setRotation(angle);
  if (immediate) mPad.setRotation(angle);
}

void CmdBoardPadEdit::rotate(const Angle& angle, const Point& center,
                             bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setPosition(
      mNewProperties.getPosition().rotated(angle, center));
  mNewProperties.setRotation(mNewProperties.getRotation() + angle);
  if (immediate) {
    mPad.setPosition(mNewProperties.getPosition());
    mPad.setRotation(mNewProperties.getRotation());
  }
}

void CmdBoardPadEdit::mirror(const Point& center, Qt::Orientation orientation,
                             bool immediate) {
  if (mNewProperties.getComponentSide() == Pad::ComponentSide::Top) {
    setComponentSideAndHoles(Pad::ComponentSide::Bottom,
                             mNewProperties.getHoles(),
                             immediate);  // can throw
  } else {
    setComponentSideAndHoles(Pad::ComponentSide::Top, mNewProperties.getHoles(),
                             immediate);  // can throw
  }

  setPosition(mNewProperties.getPosition().mirrored(orientation, center),
              immediate);
  setRotation((orientation == Qt::Horizontal)
                  ? -mNewProperties.getRotation()
                  : (Angle::deg180() - mNewProperties.getRotation()),
              immediate);
}

void CmdBoardPadEdit::setLocked(bool locked) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setLocked(locked);
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdBoardPadEdit::performExecute() {
  if (!mPad.getNetSegment()) {
    throw LogicError(__FILE__, __LINE__);  // Only board pads are mutable.
  }

  performRedo();  // can throw

  if (mNewProperties != mOldProperties) return true;
  return false;
}

void CmdBoardPadEdit::performUndo() {
  mPad.setComponentSideAndHoles(mOldProperties.getComponentSide(),
                                mOldProperties.getHoles());  // can throw

  mPad.setPosition(mOldProperties.getPosition());
  mPad.setRotation(mOldProperties.getRotation());
  mPad.setShape(mOldProperties.getShape());
  mPad.setWidth(mOldProperties.getWidth());
  mPad.setHeight(mOldProperties.getHeight());
  mPad.setRadius(mOldProperties.getRadius());
  mPad.setCustomShapeOutline(mOldProperties.getCustomShapeOutline());
  mPad.setStopMaskConfig(mOldProperties.getStopMaskConfig());
  mPad.setSolderPasteConfig(mOldProperties.getSolderPasteConfig());
  mPad.setCopperClearance(mOldProperties.getCopperClearance());
  mPad.setFunction(mOldProperties.getFunction());
  mPad.setLocked(mOldProperties.isLocked());
}

void CmdBoardPadEdit::performRedo() {
  mPad.setComponentSideAndHoles(mNewProperties.getComponentSide(),
                                mNewProperties.getHoles());  // can throw

  mPad.setPosition(mNewProperties.getPosition());
  mPad.setRotation(mNewProperties.getRotation());
  mPad.setShape(mNewProperties.getShape());
  mPad.setWidth(mNewProperties.getWidth());
  mPad.setHeight(mNewProperties.getHeight());
  mPad.setRadius(mNewProperties.getRadius());
  mPad.setCustomShapeOutline(mNewProperties.getCustomShapeOutline());
  mPad.setStopMaskConfig(mNewProperties.getStopMaskConfig());
  mPad.setSolderPasteConfig(mNewProperties.getSolderPasteConfig());
  mPad.setCopperClearance(mNewProperties.getCopperClearance());
  mPad.setFunction(mNewProperties.getFunction());
  mPad.setLocked(mNewProperties.isLocked());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
