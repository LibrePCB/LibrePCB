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
#include "boardpaddata.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardPadData::BoardPadData(const BoardPadData& other) noexcept
  : Pad(other),
    onEdited(*this),
    mLocked(other.mLocked),
    mHolesEditedSlot(*this, &BoardPadData::holesEdited) {
  mHoles.onEdited.attach(mHolesEditedSlot);
}

BoardPadData::BoardPadData(const Uuid& uuid, const BoardPadData& other) noexcept
  : BoardPadData(other) {
  mUuid = uuid;
}

BoardPadData::BoardPadData(
    const Uuid& uuid, const Point& pos, const Angle& rot, Shape shape,
    const PositiveLength& width, const PositiveLength& height,
    const UnsignedLimitedRatio& radius, const Path& customShapeOutline,
    const MaskConfig& autoStopMask, const MaskConfig& autoSolderPaste,
    const UnsignedLength& copperClearance, ComponentSide side,
    Function function, const PadHoleList& holes, bool locked) noexcept
  : Pad(uuid, pos, rot, shape, width, height, radius, customShapeOutline,
        autoStopMask, autoSolderPaste, copperClearance, side, function, holes),
    onEdited(*this),
    mLocked(locked),
    mHolesEditedSlot(*this, &BoardPadData::holesEdited) {
  mHoles.onEdited.attach(mHolesEditedSlot);
}

BoardPadData::BoardPadData(const SExpression& node)
  : Pad(node),
    onEdited(*this),
    mLocked(deserialize<bool>(node.getChild("lock/@0"))),
    mHolesEditedSlot(*this, &BoardPadData::holesEdited) {
  mHoles.onEdited.attach(mHolesEditedSlot);
}

BoardPadData::~BoardPadData() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool BoardPadData::setPosition(const Point& pos) noexcept {
  if (pos == mPosition) {
    return false;
  }

  mPosition = pos;
  onEdited.notify(Event::PositionChanged);
  return true;
}

bool BoardPadData::setRotation(const Angle& rot) noexcept {
  if (rot == mRotation) {
    return false;
  }

  mRotation = rot;
  onEdited.notify(Event::RotationChanged);
  return true;
}

bool BoardPadData::setShape(Shape shape) noexcept {
  if (shape == mShape) {
    return false;
  }

  mShape = shape;
  onEdited.notify(Event::ShapeChanged);
  return true;
}

bool BoardPadData::setWidth(const PositiveLength& width) noexcept {
  if (width == mWidth) {
    return false;
  }

  mWidth = width;
  onEdited.notify(Event::WidthChanged);
  return true;
}

bool BoardPadData::setHeight(const PositiveLength& height) noexcept {
  if (height == mHeight) {
    return false;
  }

  mHeight = height;
  onEdited.notify(Event::HeightChanged);
  return true;
}

bool BoardPadData::setRadius(const UnsignedLimitedRatio& radius) noexcept {
  if (radius == mRadius) {
    return false;
  }

  mRadius = radius;
  onEdited.notify(Event::RadiusChanged);
  return true;
}

bool BoardPadData::setCustomShapeOutline(const Path& outline) noexcept {
  if (outline == mCustomShapeOutline) {
    return false;
  }

  mCustomShapeOutline = outline;
  onEdited.notify(Event::CustomShapeOutlineChanged);
  return true;
}

bool BoardPadData::setStopMaskConfig(const MaskConfig& config) noexcept {
  if (config == mStopMaskConfig) {
    return false;
  }

  mStopMaskConfig = config;
  onEdited.notify(Event::StopMaskConfigChanged);
  return true;
}

bool BoardPadData::setSolderPasteConfig(const MaskConfig& config) noexcept {
  if (config == mSolderPasteConfig) {
    return false;
  }

  mSolderPasteConfig = config;
  onEdited.notify(Event::SolderPasteConfigChanged);
  return true;
}

bool BoardPadData::setCopperClearance(
    const UnsignedLength& clearance) noexcept {
  if (clearance == mCopperClearance) {
    return false;
  }

  mCopperClearance = clearance;
  onEdited.notify(Event::CopperClearanceChanged);
  return true;
}

bool BoardPadData::setComponentSide(ComponentSide side) noexcept {
  if (side == mComponentSide) {
    return false;
  }

  mComponentSide = side;
  onEdited.notify(Event::ComponentSideChanged);
  return true;
}

bool BoardPadData::setFunction(Function function) noexcept {
  if (function == mFunction) {
    return false;
  }

  mFunction = function;
  onEdited.notify(Event::FunctionChanged);
  return true;
}

bool BoardPadData::setLocked(bool locked) noexcept {
  if (locked == mLocked) {
    return false;
  }

  mLocked = locked;
  onEdited.notify(Event::LockedChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardPadData::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("side", mComponentSide);
  root.appendChild("shape", mShape);
  root.ensureLineBreak();
  mPosition.serialize(root.appendList("position"));
  root.appendChild("rotation", mRotation);
  Point(*mWidth, *mHeight).serialize(root.appendList("size"));
  root.appendChild("radius", mRadius);
  root.ensureLineBreak();
  root.appendChild("stop_mask", mStopMaskConfig);
  root.appendChild("solder_paste", mSolderPasteConfig);
  root.appendChild("clearance", mCopperClearance);
  root.appendChild("function", mFunction);
  root.ensureLineBreak();
  root.appendChild("lock", mLocked);
  root.ensureLineBreak();
  mCustomShapeOutline.serialize(root);
  root.ensureLineBreak();
  mHoles.serialize(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool BoardPadData::operator==(const BoardPadData& rhs) const noexcept {
  if (Pad::operator!=(rhs)) return false;
  if (mLocked != rhs.mLocked) return false;
  return true;
}

BoardPadData& BoardPadData::operator=(const BoardPadData& rhs) noexcept {
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  setPosition(rhs.mPosition);
  setRotation(rhs.mRotation);
  setShape(rhs.mShape);
  setWidth(rhs.mWidth);
  setHeight(rhs.mHeight);
  setRadius(rhs.mRadius);
  setCustomShapeOutline(rhs.mCustomShapeOutline);
  setStopMaskConfig(rhs.mStopMaskConfig);
  setSolderPasteConfig(rhs.mSolderPasteConfig);
  setCopperClearance(rhs.mCopperClearance);
  setComponentSide(rhs.mComponentSide);
  setFunction(rhs.mFunction);
  mHoles = rhs.mHoles;
  setLocked(rhs.mLocked);
  return *this;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardPadData::holesEdited(const PadHoleList& list, int index,
                               const std::shared_ptr<const PadHole>& hole,
                               PadHoleList::Event event) noexcept {
  Q_UNUSED(list);
  Q_UNUSED(index);
  Q_UNUSED(hole);
  Q_UNUSED(event);
  onEdited.notify(Event::HolesEdited);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
