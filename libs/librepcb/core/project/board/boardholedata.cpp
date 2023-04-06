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
#include "boardholedata.h"

#include "../../serialization/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardHoleData::BoardHoleData(const BoardHoleData& other) noexcept
  : mUuid(other.mUuid),
    mDiameter(other.mDiameter),
    mPath(other.mPath),
    mStopMaskConfig(other.mStopMaskConfig),
    mLocked(other.mLocked) {
}

BoardHoleData::BoardHoleData(const Uuid& uuid,
                             const BoardHoleData& other) noexcept
  : mUuid(uuid),
    mDiameter(other.mDiameter),
    mPath(other.mPath),
    mStopMaskConfig(other.mStopMaskConfig),
    mLocked(other.mLocked) {
}

BoardHoleData::BoardHoleData(const Uuid& uuid, const PositiveLength& diameter,
                             const NonEmptyPath& path,
                             const MaskConfig& stopMaskConfig, bool locked)
  : mUuid(uuid),
    mDiameter(diameter),
    mPath(path),
    mStopMaskConfig(stopMaskConfig),
    mLocked(locked) {
}

BoardHoleData::BoardHoleData(const SExpression& node)
  : mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mDiameter(deserialize<PositiveLength>(node.getChild("diameter/@0"))),
    mPath(Path(node)),
    mStopMaskConfig(deserialize<MaskConfig>(node.getChild("stop_mask/@0"))),
    mLocked(deserialize<bool>(node.getChild("lock/@0"))) {
}

BoardHoleData::~BoardHoleData() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool BoardHoleData::setUuid(const Uuid& uuid) noexcept {
  if (uuid == mUuid) {
    return false;
  }

  mUuid = uuid;
  return true;
}

bool BoardHoleData::setDiameter(const PositiveLength& diameter) noexcept {
  if (diameter == mDiameter) {
    return false;
  }

  mDiameter = diameter;
  return true;
}

bool BoardHoleData::setPath(const NonEmptyPath& path) noexcept {
  if (path == mPath) {
    return false;
  }

  mPath = path;
  return true;
}

bool BoardHoleData::setStopMaskConfig(const MaskConfig& config) noexcept {
  if (config == mStopMaskConfig) {
    return false;
  }

  mStopMaskConfig = config;
  return true;
}

bool BoardHoleData::setLocked(bool locked) noexcept {
  if (locked == mLocked) {
    return false;
  }

  mLocked = locked;
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardHoleData::serialize(SExpression& root) const {
  // Note: Keep consistent with Hole::serialize()!
  root.appendChild(mUuid);
  root.appendChild("diameter", mDiameter);
  root.ensureLineBreak();
  root.appendChild("stop_mask", mStopMaskConfig);
  root.appendChild("lock", mLocked);
  mPath->serialize(root);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool BoardHoleData::operator==(const BoardHoleData& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mDiameter != rhs.mDiameter) return false;
  if (mPath != rhs.mPath) return false;
  if (mStopMaskConfig != rhs.mStopMaskConfig) return false;
  if (mLocked != rhs.mLocked) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
