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
#include "padgeometry.h"

#include "../utils/clipperhelpers.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PadGeometry::PadGeometry(const PadGeometry& other) noexcept
  : mShape(other.mShape),
    mBaseWidth(other.mBaseWidth),
    mBaseHeight(other.mBaseHeight),
    mPath(other.mPath),
    mOffset(other.mOffset),
    mHoles(other.mHoles) {
}

PadGeometry::PadGeometry(Shape shape, const Length& width, const Length& height,
                         const Path& path, const Length& offset,
                         const HoleList& holes) noexcept
  : mShape(shape),
    mBaseWidth(width),
    mBaseHeight(height),
    mPath(path),
    mOffset(offset),
    mHoles(holes) {
}

PadGeometry::~PadGeometry() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QVector<Path> PadGeometry::toOutlines() const {
  const Length w = getWidth();
  const Length h = getHeight();
  const UnsignedLength r = getCornerRadius();

  QVector<Path> result;
  switch (mShape) {
    case Shape::Round: {
      if ((w > 0) && (h > 0)) {
        result.append(Path::obround(PositiveLength(w), PositiveLength(h)));
      }
      break;
    }
    case Shape::Rect: {
      if ((w > 0) && (h > 0)) {
        result.append(
            Path::centeredRect(PositiveLength(w), PositiveLength(h), r));
      }
      break;
    }
    case Shape::Octagon: {
      if ((w > 0) && (h > 0)) {
        result.append(Path::octagon(PositiveLength(w), PositiveLength(h), r));
      }
      break;
    }
    case Shape::Stroke: {
      if (w > 0) {
        result = mPath.toOutlineStrokes(PositiveLength(w));
        // Unite all outlines to get only a single, non-intersecting outline.
        // Not needed if there's only one straight line segment since it
        // cannot be self-intersecting.
        if ((result.count() > 1) ||
            ((result.count() == 1) &&
             (mPath.getVertices().first().getAngle() != Angle::deg0()))) {
          ClipperLib::Paths paths =
              ClipperHelpers::convert(result, maxArcTolerance());
          std::unique_ptr<ClipperLib::PolyTree> tree =
              ClipperHelpers::uniteToTree(paths,
                                          ClipperLib::pftNonZero);  // can throw
          paths = ClipperHelpers::flattenTree(*tree);  // can throw
          result = ClipperHelpers::convert(paths);
        }
      }
      break;
    }
    default: {
      qCritical() << "Unhandled switch-case in PadGeometry::toOutlines():"
                  << static_cast<int>(mShape);
      Q_ASSERT(false);
      break;
    }
  }
  return result;
}

QPainterPath PadGeometry::toQPainterPathPx() const noexcept {
  const QPainterPath area = toFilledQPainterPathPx();
  if (area.isEmpty()) {
    return QPainterPath();
  }

  QPainterPath p;
  p.setFillRule(Qt::OddEvenFill);  // Important to subtract the holes!
  p.addPath(area);
  p.addPath(toHolesQPainterPathPx());
  return p;
}

QPainterPath PadGeometry::toFilledQPainterPathPx() const noexcept {
  QPainterPath p;
  try {
    foreach (const Path& outline, toOutlines()) {
      p.addPath(outline.toQPainterPathPx());
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to build pad painter path:" << e.getMsg();
  }
  return p;
}

QPainterPath PadGeometry::toHolesQPainterPathPx() const noexcept {
  QPainterPath p;
  p.setFillRule(Qt::WindingFill);
  for (const Hole& hole : mHoles) {
    for (const Path& path :
         hole.getPath()->toOutlineStrokes(hole.getDiameter())) {
      p.addPath(path.toQPainterPathPx());
    }
  }
  return p;
}

PadGeometry PadGeometry::withOffset(const Length& offset) const noexcept {
  return PadGeometry(mShape, mBaseWidth, mBaseHeight, mPath, mOffset + offset,
                     mHoles);
}

PadGeometry PadGeometry::withoutHoles() const noexcept {
  return PadGeometry(mShape, mBaseWidth, mBaseHeight, mPath, mOffset,
                     HoleList{});
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

PadGeometry PadGeometry::round(const PositiveLength& width,
                               const PositiveLength& height,
                               const HoleList& holes) noexcept {
  return PadGeometry(Shape::Round, *width, *height, Path(), Length(0), holes);
}

PadGeometry PadGeometry::rect(const PositiveLength& width,
                              const PositiveLength& height,
                              const HoleList& holes) noexcept {
  return PadGeometry(Shape::Rect, *width, *height, Path(), Length(0), holes);
}

PadGeometry PadGeometry::octagon(const PositiveLength& width,
                                 const PositiveLength& height,
                                 const HoleList& holes) noexcept {
  return PadGeometry(Shape::Octagon, *width, *height, Path(), Length(0), holes);
}

PadGeometry PadGeometry::stroke(const PositiveLength& diameter,
                                const NonEmptyPath& path,
                                const HoleList& holes) noexcept {
  return PadGeometry(Shape::Stroke, *diameter, Length(0), *path, Length(0),
                     holes);
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool PadGeometry::operator==(const PadGeometry& rhs) const noexcept {
  if (mShape != rhs.mShape) return false;
  if (mBaseWidth != rhs.mBaseWidth) return false;
  if (mBaseHeight != rhs.mBaseHeight) return false;
  if (mPath != rhs.mPath) return false;
  if (mOffset != rhs.mOffset) return false;
  if (mHoles != rhs.mHoles) return false;
  return true;
}

PadGeometry& PadGeometry::operator=(const PadGeometry& rhs) noexcept {
  mShape = rhs.mShape;
  mBaseWidth = rhs.mBaseWidth;
  mBaseHeight = rhs.mBaseHeight;
  mPath = rhs.mPath;
  mOffset = rhs.mOffset;
  mHoles = rhs.mHoles;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
