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
#include "polygon.h"

#include "../utils/toolbox.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Polygon::Polygon(const Polygon& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mLayer(other.mLayer),
    mLineWidth(other.mLineWidth),
    mIsFilled(other.mIsFilled),
    mIsGrabArea(other.mIsGrabArea),
    mPath(other.mPath) {
}

Polygon::Polygon(const Uuid& uuid, const Polygon& other) noexcept
  : Polygon(other) {
  mUuid = uuid;
}

Polygon::Polygon(const Uuid& uuid, const Layer& layer,
                 const UnsignedLength& lineWidth, bool fill, bool isGrabArea,
                 const Path& path) noexcept
  : onEdited(*this),
    mUuid(uuid),
    mLayer(&layer),
    mLineWidth(lineWidth),
    mIsFilled(fill),
    mIsGrabArea(isGrabArea),
    mPath(path) {
}

Polygon::Polygon(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mLayer(deserialize<const Layer*>(node.getChild("layer/@0"))),
    mLineWidth(deserialize<UnsignedLength>(node.getChild("width/@0"))),
    mIsFilled(deserialize<bool>(node.getChild("fill/@0"))),
    mIsGrabArea(deserialize<bool>(node.getChild("grab_area/@0"))),
    mPath(node) {
}

Polygon::~Polygon() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool Polygon::setLayer(const Layer& layer) noexcept {
  if (&layer == mLayer) {
    return false;
  }

  mLayer = &layer;
  onEdited.notify(Event::LayerChanged);
  return true;
}

bool Polygon::setLineWidth(const UnsignedLength& width) noexcept {
  if (width == mLineWidth) {
    return false;
  }

  mLineWidth = width;
  onEdited.notify(Event::LineWidthChanged);
  return true;
}

bool Polygon::setIsFilled(bool isFilled) noexcept {
  if (isFilled == mIsFilled) {
    return false;
  }

  mIsFilled = isFilled;
  onEdited.notify(Event::IsFilledChanged);
  return true;
}

bool Polygon::setIsGrabArea(bool isGrabArea) noexcept {
  if (isGrabArea == mIsGrabArea) {
    return false;
  }

  mIsGrabArea = isGrabArea;
  onEdited.notify(Event::IsGrabAreaChanged);
  return true;
}

bool Polygon::setPath(const Path& path) noexcept {
  if (path == mPath) {
    return false;
  }

  mPath = path;
  onEdited.notify(Event::PathChanged);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Polygon::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("layer", *mLayer);
  root.ensureLineBreak();
  root.appendChild("width", mLineWidth);
  root.appendChild("fill", mIsFilled);
  root.appendChild("grab_area", mIsGrabArea);
  root.ensureLineBreak();
  mPath.serialize(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Polygon::operator==(const Polygon& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mLayer != rhs.mLayer) return false;
  if (mLineWidth != rhs.mLineWidth) return false;
  if (mIsFilled != rhs.mIsFilled) return false;
  if (mIsGrabArea != rhs.mIsGrabArea) return false;
  if (mPath != rhs.mPath) return false;
  return true;
}

Polygon& Polygon::operator=(const Polygon& rhs) noexcept {
  if (mUuid != rhs.mUuid) {
    mUuid = rhs.mUuid;
    onEdited.notify(Event::UuidChanged);
  }
  setLayer(*rhs.mLayer);
  setLineWidth(rhs.mLineWidth);
  setIsFilled(rhs.mIsFilled);
  setIsGrabArea(rhs.mIsGrabArea);
  setPath(rhs.mPath);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
