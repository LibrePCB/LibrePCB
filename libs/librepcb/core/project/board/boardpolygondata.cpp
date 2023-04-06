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
#include "boardpolygondata.h"

#include "../../serialization/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardPolygonData::BoardPolygonData(const BoardPolygonData& other) noexcept
  : mUuid(other.mUuid),
    mLayer(other.mLayer),
    mLineWidth(other.mLineWidth),
    mPath(other.mPath),
    mIsFilled(other.mIsFilled),
    mIsGrabArea(other.mIsGrabArea),
    mLocked(other.mLocked) {
}

BoardPolygonData::BoardPolygonData(const Uuid& uuid,
                                   const BoardPolygonData& other) noexcept
  : mUuid(uuid),
    mLayer(other.mLayer),
    mLineWidth(other.mLineWidth),
    mPath(other.mPath),
    mIsFilled(other.mIsFilled),
    mIsGrabArea(other.mIsGrabArea),
    mLocked(other.mLocked) {
}

BoardPolygonData::BoardPolygonData(const Uuid& uuid, const Layer& layer,
                                   const UnsignedLength& lineWidth,
                                   const Path& path, bool fill, bool isGrabArea,
                                   bool locked) noexcept
  : mUuid(uuid),
    mLayer(&layer),
    mLineWidth(lineWidth),
    mPath(path),
    mIsFilled(fill),
    mIsGrabArea(isGrabArea),
    mLocked(locked) {
}

BoardPolygonData::BoardPolygonData(const SExpression& node)
  : mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mLayer(deserialize<const Layer*>(node.getChild("layer/@0"))),
    mLineWidth(deserialize<UnsignedLength>(node.getChild("width/@0"))),
    mPath(node),
    mIsFilled(deserialize<bool>(node.getChild("fill/@0"))),
    mIsGrabArea(deserialize<bool>(node.getChild("grab_area/@0"))),
    mLocked(deserialize<bool>(node.getChild("lock/@0"))) {
}

BoardPolygonData::~BoardPolygonData() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool BoardPolygonData::setLayer(const Layer& layer) noexcept {
  if (&layer == mLayer) {
    return false;
  }

  mLayer = &layer;
  return true;
}

bool BoardPolygonData::setLineWidth(const UnsignedLength& width) noexcept {
  if (width == mLineWidth) {
    return false;
  }

  mLineWidth = width;
  return true;
}

bool BoardPolygonData::setPath(const Path& path) noexcept {
  if (path == mPath) {
    return false;
  }

  mPath = path;
  return true;
}

bool BoardPolygonData::setIsFilled(bool isFilled) noexcept {
  if (isFilled == mIsFilled) {
    return false;
  }

  mIsFilled = isFilled;
  return true;
}

bool BoardPolygonData::setIsGrabArea(bool isGrabArea) noexcept {
  if (isGrabArea == mIsGrabArea) {
    return false;
  }

  mIsGrabArea = isGrabArea;
  return true;
}

bool BoardPolygonData::setLocked(bool locked) noexcept {
  if (locked == mLocked) {
    return false;
  }

  mLocked = locked;
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardPolygonData::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("layer", *mLayer);
  root.ensureLineBreak();
  root.appendChild("width", mLineWidth);
  root.appendChild("fill", mIsFilled);
  root.appendChild("grab_area", mIsGrabArea);
  root.appendChild("lock", mLocked);
  root.ensureLineBreak();
  mPath.serialize(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool BoardPolygonData::operator==(const BoardPolygonData& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mLayer != rhs.mLayer) return false;
  if (mLineWidth != rhs.mLineWidth) return false;
  if (mPath != rhs.mPath) return false;
  if (mIsFilled != rhs.mIsFilled) return false;
  if (mIsGrabArea != rhs.mIsGrabArea) return false;
  if (mLocked != rhs.mLocked) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
