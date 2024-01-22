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
    mRadius(other.mRadius),
    mPath(other.mPath),
    mOffset(other.mOffset),
    mHoles(other.mHoles) {
}

PadGeometry::PadGeometry(Shape shape, const Length& width, const Length& height,
                         const UnsignedLimitedRatio& radius, const Path& path,
                         const Length& offset,
                         const PadHoleList& holes) noexcept
  : mShape(shape),
    mBaseWidth(width),
    mBaseHeight(height),
    mRadius(radius),
    mPath(path),
    mOffset(offset),
    mHoles(holes) {
}

PadGeometry::~PadGeometry() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

UnsignedLength PadGeometry::getCornerRadius() const noexcept {
  const Length size = std::min(mBaseWidth, mBaseHeight) / 2;
  const Length radius = qBound(
      Length(0), Length::fromMm(size.toMm() * mRadius->toNormalized()), size);
  return UnsignedLength(std::max(Length(0), radius + mOffset));
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QVector<Path> PadGeometry::toOutlines() const {
  const Length w = getWidth();
  const Length h = getHeight();
  const UnsignedLength r = getCornerRadius();

  QVector<Path> result;
  switch (mShape) {
    case Shape::RoundedRect: {
      if ((w > 0) && (h > 0)) {
        result.append(
            Path::centeredRect(PositiveLength(w), PositiveLength(h), r));
      }
      break;
    }
    case Shape::RoundedOctagon: {
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
    case Shape::Custom: {
      const Path outline = mPath.toClosedPath();
      if (outline.getVertices().count() >= 3) {
        // Note: If mOffset is zero, the offset operation sounds superfluous.
        // However, this operation ensures that invalid outlines (e.g.
        // overlaps or intersections) will be cleaned before any further
        // processing of the pad shape (e.g. Gerber export).
        ClipperLib::Paths paths{
            ClipperHelpers::convert(outline, maxArcTolerance())};
        std::unique_ptr<ClipperLib::PolyTree> tree =
            ClipperHelpers::offsetToTree(paths, mOffset,
                                         maxArcTolerance());  // can throw
        paths = ClipperHelpers::flattenTree(*tree);  // can throw
        result = ClipperHelpers::convert(paths);
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
  for (const PadHole& hole : mHoles) {
    for (const Path& path :
         hole.getPath()->toOutlineStrokes(hole.getDiameter())) {
      p.addPath(path.toQPainterPathPx());
    }
  }
  return p;
}

PadGeometry PadGeometry::withOffset(const Length& offset) const noexcept {
  return PadGeometry(mShape, mBaseWidth, mBaseHeight, mRadius, mPath,
                     mOffset + offset, mHoles);
}

PadGeometry PadGeometry::withoutHoles() const noexcept {
  return PadGeometry(mShape, mBaseWidth, mBaseHeight, mRadius, mPath, mOffset,
                     PadHoleList{});
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

PadGeometry PadGeometry::roundedRect(const PositiveLength& width,
                                     const PositiveLength& height,
                                     const UnsignedLimitedRatio& radius,
                                     const PadHoleList& holes) noexcept {
  return PadGeometry(Shape::RoundedRect, *width, *height, radius, Path(),
                     Length(0), holes);
}

PadGeometry PadGeometry::roundedOctagon(const PositiveLength& width,
                                        const PositiveLength& height,
                                        const UnsignedLimitedRatio& radius,
                                        const PadHoleList& holes) noexcept {
  return PadGeometry(Shape::RoundedOctagon, *width, *height, radius, Path(),
                     Length(0), holes);
}

PadGeometry PadGeometry::stroke(const PositiveLength& diameter,
                                const NonEmptyPath& path,
                                const PadHoleList& holes) noexcept {
  return PadGeometry(Shape::Stroke, *diameter, Length(0),
                     UnsignedLimitedRatio(Ratio::fromPercent(0)), *path,
                     Length(0), holes);
}

PadGeometry PadGeometry::custom(const Path& outline, const PadHoleList& holes) {
  return PadGeometry(Shape::Custom, Length(0), Length(0),
                     UnsignedLimitedRatio(Ratio::fromPercent(0)), outline,
                     Length(0), holes);
}

bool PadGeometry::isValidCustomOutline(const Path& path) noexcept {
  return std::abs(ClipperLib::Area(ClipperHelpers::convert(
             path.toClosedPath(), maxArcTolerance()))) > 1;
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool PadGeometry::operator==(const PadGeometry& rhs) const noexcept {
  if (mShape != rhs.mShape) return false;
  if (mBaseWidth != rhs.mBaseWidth) return false;
  if (mBaseHeight != rhs.mBaseHeight) return false;
  if (mRadius != rhs.mRadius) return false;
  if (mPath != rhs.mPath) return false;
  if (mOffset != rhs.mOffset) return false;
  if (mHoles != rhs.mHoles) return false;
  return true;
}

PadGeometry& PadGeometry::operator=(const PadGeometry& rhs) noexcept {
  mShape = rhs.mShape;
  mBaseWidth = rhs.mBaseWidth;
  mBaseHeight = rhs.mBaseHeight;
  mRadius = rhs.mRadius;
  mPath = rhs.mPath;
  mOffset = rhs.mOffset;
  mHoles = rhs.mHoles;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
