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

#include "../types/layer.h"

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

Angle Transform::mapMirrorable(const Angle& angle) const noexcept {
  return mMirrored ? (mRotation - angle) : (mRotation + angle);
}

Angle Transform::mapNonMirrorable(const Angle& angle) const noexcept {
  return mRotation + (mMirrored ? (Angle::deg180() - angle) : angle);
}

Point Transform::map(const Point& point) const noexcept {
  Point p = point;
  if (mMirrored) {
    p.mirror(Qt::Horizontal);
  }
  if (mRotation) {
    p.rotate(mRotation);
  }
  return p + mPosition;
}

Path Transform::map(const Path& path) const noexcept {
  Path p = path;
  if (mMirrored) {
    p.mirror(Qt::Horizontal);
  }
  if (mRotation) {
    p.rotate(mRotation);
  }
  if (!mPosition.isOrigin()) {
    p.translate(mPosition);
  }
  return p;
}

NonEmptyPath Transform::map(const NonEmptyPath& path) const noexcept {
  return NonEmptyPath(map(*path));
}

const Layer& Transform::map(const Layer& layer) const noexcept {
  return mMirrored ? layer.mirrored() : layer;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
