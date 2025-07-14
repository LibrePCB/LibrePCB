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

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CmdFootprintPadEdit::CmdFootprintPadEdit(FootprintPad& pad) noexcept
  : UndoCommand(tr("Edit footprint pad")),
    mPad(pad),
    mOldProperties(mPad),
    mNewProperties(mOldProperties) {
}

CmdFootprintPadEdit::~CmdFootprintPadEdit() noexcept {
  if (!wasEverExecuted()) {
    try {
      performUndo();
    } catch (...) {
      qCritical() << "Undo failed in CmdFootprintPadEdit destructor!";
    }
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void CmdFootprintPadEdit::setPackagePadUuid(const std::optional<Uuid>& pad,
                                            bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setPackagePadUuid(pad);
  if (immediate) mPad.setPackagePadUuid(pad);
}

void CmdFootprintPadEdit::setComponentSide(FootprintPad::ComponentSide side,
                                           bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setComponentSide(side);
  if (immediate) mPad.setComponentSide(side);
}

void CmdFootprintPadEdit::setFunction(FootprintPad::Function function,
                                      bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setFunction(function);
  if (immediate) mPad.setFunction(function);
}

void CmdFootprintPadEdit::setShape(FootprintPad::Shape shape,
                                   bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setShape(shape);
  if (immediate) mPad.setShape(shape);
}

void CmdFootprintPadEdit::setWidth(const PositiveLength& width,
                                   bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setWidth(width);
  if (immediate) mPad.setWidth(width);
}

void CmdFootprintPadEdit::setHeight(const PositiveLength& height,
                                    bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setHeight(height);
  if (immediate) mPad.setHeight(height);
}

void CmdFootprintPadEdit::setRadius(const UnsignedLimitedRatio& radius,
                                    bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setRadius(radius);
  if (immediate) mPad.setRadius(radius);
}

void CmdFootprintPadEdit::setCustomShapeOutline(const Path& outline) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setCustomShapeOutline(outline);
}

void CmdFootprintPadEdit::setStopMaskConfig(const MaskConfig& config,
                                            bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setStopMaskConfig(config);
  if (immediate) mPad.setStopMaskConfig(config);
}

void CmdFootprintPadEdit::setSolderPasteConfig(
    const MaskConfig& config) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setSolderPasteConfig(config);
}

void CmdFootprintPadEdit::setCopperClearance(const UnsignedLength& clearance,
                                             bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setCopperClearance(clearance);
  if (immediate) mPad.setCopperClearance(clearance);
}

void CmdFootprintPadEdit::setPosition(const Point& pos,
                                      bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setPosition(pos);
  if (immediate) mPad.setPosition(pos);
}

void CmdFootprintPadEdit::translate(const Point& deltaPos,
                                    bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setPosition(mNewProperties.getPosition() + deltaPos);
  if (immediate) mPad.setPosition(mNewProperties.getPosition());
}

void CmdFootprintPadEdit::snapToGrid(const PositiveLength& gridInterval,
                                     bool immediate) noexcept {
  setPosition(mNewProperties.getPosition().mappedToGrid(gridInterval),
              immediate);
}

void CmdFootprintPadEdit::setRotation(const Angle& angle,
                                      bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setRotation(angle);
  if (immediate) mPad.setRotation(angle);
}

void CmdFootprintPadEdit::rotate(const Angle& angle, const Point& center,
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

void CmdFootprintPadEdit::mirrorGeometry(Qt::Orientation orientation,
                                         const Point& center,
                                         bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.setPosition(
      mNewProperties.getPosition().mirrored(orientation, center));
  if (orientation == Qt::Horizontal) {
    mNewProperties.setRotation(Angle::deg180() - mNewProperties.getRotation());
  } else {
    mNewProperties.setRotation(-mNewProperties.getRotation());
  }
  mNewProperties.setCustomShapeOutline(
      mNewProperties.getCustomShapeOutline().mirrored(orientation));
  if (immediate) {
    mPad.setPosition(mNewProperties.getPosition());
    mPad.setRotation(mNewProperties.getRotation());
    mPad.setCustomShapeOutline(mNewProperties.getCustomShapeOutline());
  }
}

void CmdFootprintPadEdit::mirrorLayer(bool immediate) noexcept {
  if (mNewProperties.getComponentSide() == FootprintPad::ComponentSide::Top) {
    mNewProperties.setComponentSide(FootprintPad::ComponentSide::Bottom);
  } else {
    mNewProperties.setComponentSide(FootprintPad::ComponentSide::Top);
  }
  if (immediate) {
    mPad.setComponentSide(mNewProperties.getComponentSide());
  }
}

void CmdFootprintPadEdit::setHoles(const PadHoleList& holes,
                                   bool immediate) noexcept {
  Q_ASSERT(!wasEverExecuted());
  mNewProperties.getHoles() = holes;
  if (immediate) {
    mPad.getHoles() = holes;
  }
}

/*******************************************************************************
 *  Inherited from UndoCommand
 ******************************************************************************/

bool CmdFootprintPadEdit::performExecute() {
  performRedo();  // can throw

  if (mNewProperties != mOldProperties) return true;
  return false;
}

void CmdFootprintPadEdit::performUndo() {
  mPad = mOldProperties;
}

void CmdFootprintPadEdit::performRedo() {
  mPad = mNewProperties;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
