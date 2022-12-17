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
#include "transform.h"

#include "../graphics/graphicslayer.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool Transform::map(bool mirror) const noexcept {
  return mMirrored ? !mirror : mirror;
}

Angle Transform::map(const Angle& angle) const noexcept {
  Angle a = mRotation + angle;
  return mMirrored ? (Angle::deg180() - a) : a;
}

Point Transform::map(const Point& point) const noexcept {
  Point p = point;
  if (mRotation) {
    p.rotate(mRotation);
  }
  if (mMirrored) {
    p.mirror(Qt::Horizontal);
  }
  return p + mPosition;
}

Path Transform::map(const Path& path) const noexcept {
  Path p = path;
  if (mRotation) {
    p.rotate(mRotation);
  }
  if (mMirrored) {
    p.mirror(Qt::Horizontal);
  }
  if (!mPosition.isOrigin()) {
    p.translate(mPosition);
  }
  return p;
}

NonEmptyPath Transform::map(const NonEmptyPath& path) const noexcept {
  return NonEmptyPath(map(*path));
}

QString Transform::map(const QString& layerName) const noexcept {
  return mMirrored ? GraphicsLayer::getMirroredLayerName(layerName) : layerName;
}

GraphicsLayerName Transform::map(const GraphicsLayerName& layerName) const
    noexcept {
  return GraphicsLayerName(map(*layerName));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
