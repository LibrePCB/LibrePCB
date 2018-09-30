/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#include "../toolbox.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Polygon::Polygon(const Polygon& other) noexcept
  : mUuid(other.mUuid),
    mLayerName(other.mLayerName),
    mLineWidth(other.mLineWidth),
    mIsFilled(other.mIsFilled),
    mIsGrabArea(other.mIsGrabArea),
    mPath(other.mPath) {
}

Polygon::Polygon(const Uuid& uuid, const Polygon& other) noexcept
  : Polygon(other) {
  mUuid = uuid;
}

Polygon::Polygon(const Uuid& uuid, const GraphicsLayerName& layerName,
                 const UnsignedLength& lineWidth, bool fill, bool isGrabArea,
                 const Path& path) noexcept
  : mUuid(uuid),
    mLayerName(layerName),
    mLineWidth(lineWidth),
    mIsFilled(fill),
    mIsGrabArea(isGrabArea),
    mPath(path) {
}

Polygon::Polygon(const SExpression& node)
  : mUuid(Uuid::createRandom()),  // backward compatibility, remove this some
                                  // time!
    mLayerName(node.getValueByPath<GraphicsLayerName>("layer", true)),
    mLineWidth(node.getValueByPath<UnsignedLength>("width")),
    mIsFilled(node.getValueByPath<bool>("fill")),
    mIsGrabArea(false),
    mPath() {
  if (node.getChildByIndex(0).isString()) {
    mUuid = node.getChildByIndex(0).getValue<Uuid>();
  }
  if (node.tryGetChildByPath("grab_area")) {
    mIsGrabArea = node.getValueByPath<bool>("grab_area");
  } else {
    // backward compatibility, remove this some time!
    mIsGrabArea = node.getValueByPath<bool>("grab");
  }

  // load vertices
  if ((!node.tryGetChildByPath("pos")) &&
      (!node.tryGetChildByPath("position"))) {
    mPath = Path(node);  // can throw
  } else {
    // backward compatibility, remove this some time!
    mPath.addVertex(Point(node.getChildByPath("pos")));
    foreach (const SExpression& child, node.getChildren("segment")) {
      mPath.getVertices().last().setAngle(child.getValueByPath<Angle>("angle"));
      mPath.addVertex(Point(child.getChildByPath("pos")));
    }
  }
}

Polygon::~Polygon() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void Polygon::setLayerName(const GraphicsLayerName& name) noexcept {
  if (name == mLayerName) return;
  mLayerName = name;
  foreach (IF_PolygonObserver* object, mObservers) {
    object->polygonLayerNameChanged(mLayerName);
  }
}

void Polygon::setLineWidth(const UnsignedLength& width) noexcept {
  if (width == mLineWidth) return;
  mLineWidth = width;
  foreach (IF_PolygonObserver* object, mObservers) {
    object->polygonLineWidthChanged(mLineWidth);
  }
}

void Polygon::setIsFilled(bool isFilled) noexcept {
  if (isFilled == mIsFilled) return;
  mIsFilled = isFilled;
  foreach (IF_PolygonObserver* object, mObservers) {
    object->polygonIsFilledChanged(mIsFilled);
  }
}

void Polygon::setIsGrabArea(bool isGrabArea) noexcept {
  if (isGrabArea == mIsGrabArea) return;
  mIsGrabArea = isGrabArea;
  foreach (IF_PolygonObserver* object, mObservers) {
    object->polygonIsGrabAreaChanged(mIsGrabArea);
  }
}

void Polygon::setPath(const Path& path) noexcept {
  if (path == mPath) return;
  mPath = path;
  foreach (IF_PolygonObserver* object, mObservers) {
    object->polygonPathChanged(mPath);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Polygon::registerObserver(IF_PolygonObserver& object) const noexcept {
  mObservers.insert(&object);
}

void Polygon::unregisterObserver(IF_PolygonObserver& object) const noexcept {
  mObservers.remove(&object);
}

void Polygon::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("layer", mLayerName, false);
  root.appendChild("width", mLineWidth, true);
  root.appendChild("fill", mIsFilled, false);
  root.appendChild("grab_area", mIsGrabArea, false);
  mPath.serialize(root);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Polygon::operator==(const Polygon& rhs) const noexcept {
  if (mUuid != rhs.mUuid) return false;
  if (mLayerName != rhs.mLayerName) return false;
  if (mLineWidth != rhs.mLineWidth) return false;
  if (mIsFilled != rhs.mIsFilled) return false;
  if (mIsGrabArea != rhs.mIsGrabArea) return false;
  if (mPath != rhs.mPath) return false;
  return true;
}

Polygon& Polygon::operator=(const Polygon& rhs) noexcept {
  mUuid       = rhs.mUuid;
  mLayerName  = rhs.mLayerName;
  mLineWidth  = rhs.mLineWidth;
  mIsFilled   = rhs.mIsFilled;
  mIsGrabArea = rhs.mIsGrabArea;
  mPath       = rhs.mPath;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
