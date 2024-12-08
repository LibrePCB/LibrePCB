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
#include "hole.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Hole::Hole(const Hole& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mDiameter(other.mDiameter),
    mPath(other.mPath),
    mStopMaskConfig(other.mStopMaskConfig) {
}

Hole::Hole(const Uuid& uuid, const Hole& other) noexcept : Hole(other) {
  mUuid = uuid;
}

Hole::Hole(const Uuid& uuid, const PositiveLength& diameter,
           const NonEmptyPath& path, const MaskConfig& stopMaskConfig) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mDiameter(diameter),
    mPath(path),
    mStopMaskConfig(stopMaskConfig) {
}

Hole::Hole(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mDiameter(deserialize<PositiveLength>(node.getChild("diameter/@0"))),
    mPath(Path(node)),
    mStopMaskConfig(deserialize<MaskConfig>(node.getChild("stop_mask/@0"))) {
}

Hole::~Hole() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool Hole::isSlot() const noexcept {
  return mPath->getVertices().count() > 1;
}

bool Hole::isMultiSegmentSlot() const noexcept {
  return mPath->getVertices().count() > 2;
}

bool Hole::isCurvedSlot() const noexcept {
  return mPath->isCurved();
}

std::optional<Length> Hole::getPreviewStopMaskOffset() const noexcept {
  std::optional<Length> offset;
  if (!mStopMaskConfig.isEnabled()) {
    offset = std::nullopt;
  } else if (auto value = mStopMaskConfig.getOffset()) {
    offset = *value;
  } else {
    offset = Length(100000);  // Only for preview.
  }
  return offset;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool Hole::setDiameter(const PositiveLength& diameter) noexcept {
  if (diameter == mDiameter) {
    return false;
  }

  mDiameter = diameter;
  onEdited.notify(Event::DiameterChanged);
  return true;
}

bool Hole::setPath(const NonEmptyPath& path) noexcept {
  if (path == mPath) {
    return false;
  }

  mPath = path;
  onEdited.notify(Event::PathChanged);
  return true;
}

bool Hole::setStopMaskConfig(const MaskConfig& config) noexcept {
  if (config == mStopMaskConfig) {
    return false;
  }

  mStopMaskConfig = config;
  onEdited.notify(Event::StopMaskConfigChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Hole::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("diameter", mDiameter);
  root.ensureLineBreak();
  root.appendChild("stop_mask", mStopMaskConfig);
  mPath->serialize(root);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Hole::operator==(const Hole& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mDiameter != rhs.mDiameter) return false;
  if (mPath != rhs.mPath) return false;
  if (mStopMaskConfig != rhs.mStopMaskConfig) return false;
  return true;
}

Hole& Hole::operator=(const Hole& rhs) noexcept {
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  setDiameter(mDiameter = rhs.mDiameter);
  setPath(rhs.mPath);
  setStopMaskConfig(rhs.mStopMaskConfig);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
