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
#include "path.h"

#include "../serialization/sexpression.h"
#include "../utils/toolbox.h"

#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Path::Path(const Path& other) noexcept
  : mVertices(other.mVertices), mPainterPathPx(other.mPainterPathPx) {
}

Path::Path(const SExpression& node) {
  foreach (const SExpression* child, node.getChildren("vertex")) {
    mVertices.append(Vertex(*child));
  }
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool Path::isClosed() const noexcept {
  if (mVertices.count() >= 2) {
    return (mVertices.first().getPos() == mVertices.last().getPos());
  } else {
    return false;
  }
}

UnsignedLength Path::getTotalStraightLength() const noexcept {
  UnsignedLength length(0);
  if (mVertices.count() >= 2) {
    Point lastPos = mVertices.first().getPos();
    for (int i = 1; i < mVertices.count(); ++i) {
      const Point& pos = mVertices.at(i).getPos();
      length += (pos - lastPos).getLength();
      lastPos = pos;
    }
  }
  return length;
}

Point Path::calcNearestPointBetweenVertices(const Point& p) const noexcept {
  if (!mVertices.isEmpty()) {
    // Note: Does not take arcs into account yet.
    Point nearest = mVertices.first().getPos();
    for (int i = 1; i < mVertices.count(); ++i) {
      Point tmp = Toolbox::nearestPointOnLine(p, mVertices.at(i - 1).getPos(),
                                              mVertices.at(i).getPos());
      if ((tmp - p).getLength() < (nearest - p).getLength()) {
        nearest = tmp;
      }
    }
    return nearest;
  } else {
    return Point();
  }
}

Path Path::toClosedPath() const noexcept {
  Path p(*this);
  p.close();
  return p;
}

QVector<Path> Path::toOutlineStrokes(const PositiveLength& width) const
    noexcept {
  QVector<Path> paths;
  paths.reserve(mVertices.count());
  for (int i = 1; i < mVertices.count(); ++i) {  // skip first vertex!
    const Vertex& v = mVertices.at(i);
    const Vertex& v0 = mVertices.at(i - 1);
    if (v0.getAngle() == 0) {
      paths.append(obround(v0.getPos(), v.getPos(), width));
    } else {
      paths.append(arcObround(v0.getPos(), v.getPos(), v0.getAngle(), width));
    }
  }
  return paths;
}

const QPainterPath& Path::toQPainterPathPx() const noexcept {
  if (mPainterPathPx.isEmpty()) {
    for (int i = 0; i < mVertices.count(); ++i) {
      const Vertex& v = mVertices.at(i);
      if (i == 0) {
        mPainterPathPx.moveTo(v.getPos().toPxQPointF());
        continue;
      }
      const Vertex& v0 = mVertices.at(i - 1);
      if (v0.getAngle() == 0) {
        mPainterPathPx.lineTo(v.getPos().toPxQPointF());
      } else {
        QPointF centerPx =
            Toolbox::arcCenter(v0.getPos(), v.getPos(), v0.getAngle())
                .toPxQPointF();
        qreal radiusPx =
            Toolbox::arcRadius(v0.getPos(), v.getPos(), v0.getAngle())
                .abs()
                .toPx();
        QPointF diffPx = v0.getPos().toPxQPointF() - centerPx;
        qreal startAngleDeg =
            -qRadiansToDegrees(qAtan2(diffPx.y(), diffPx.x()));
        mPainterPathPx.arcTo(centerPx.x() - radiusPx, centerPx.y() - radiusPx,
                             radiusPx * 2, radiusPx * 2, startAngleDeg,
                             v0.getAngle().toDeg());
      }
    }
  }
  return mPainterPathPx;
}

/*******************************************************************************
 *  Transformations
 ******************************************************************************/

Path& Path::translate(const Point& offset) noexcept {
  for (Vertex& vertex : mVertices) {
    vertex.setPos(vertex.getPos() + offset);
  }
  invalidatePainterPath();
  return *this;
}

Path Path::translated(const Point& offset) const noexcept {
  return Path(*this).translate(offset);
}

Path& Path::mapToGrid(const PositiveLength& gridInterval) noexcept {
  for (Vertex& vertex : mVertices) {
    vertex.setPos(vertex.getPos().mappedToGrid(gridInterval));
  }
  invalidatePainterPath();
  return *this;
}

Path Path::mappedToGrid(const PositiveLength& gridInterval) const noexcept {
  return Path(*this).mapToGrid(gridInterval);
}

Path& Path::rotate(const Angle& angle, const Point& center) noexcept {
  for (Vertex& vertex : mVertices) {
    vertex.setPos(vertex.getPos().rotated(angle, center));
  }
  invalidatePainterPath();
  return *this;
}

Path Path::rotated(const Angle& angle, const Point& center) const noexcept {
  return Path(*this).rotate(angle, center);
}

Path& Path::mirror(Qt::Orientation orientation, const Point& center) noexcept {
  for (Vertex& vertex : mVertices) {
    vertex.setPos(vertex.getPos().mirrored(orientation, center));
    vertex.setAngle(-vertex.getAngle());
  }
  invalidatePainterPath();
  return *this;
}

Path Path::mirrored(Qt::Orientation orientation, const Point& center) const
    noexcept {
  return Path(*this).mirror(orientation, center);
}

Path& Path::reverse() noexcept {
  QVector<Vertex> vertices;
  vertices.reserve(mVertices.count());
  for (int i = mVertices.count() - 1; i >= 0; --i) {
    vertices.append(
        Vertex(mVertices.at(i).getPos(), -mVertices.value(i - 1).getAngle()));
  }
  mVertices = vertices;
  invalidatePainterPath();
  return *this;
}

Path Path::reversed() const noexcept {
  return Path(*this).reverse();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Path::addVertex(const Vertex& vertex) noexcept {
  mVertices.append(vertex);
  invalidatePainterPath();
}

void Path::addVertex(const Point& pos, const Angle& angle) noexcept {
  addVertex(Vertex(pos, angle));
}

void Path::insertVertex(int index, const Vertex& vertex) noexcept {
  mVertices.insert(index, vertex);
  invalidatePainterPath();
}

void Path::insertVertex(int index, const Point& pos,
                        const Angle& angle) noexcept {
  insertVertex(index, Vertex(pos, angle));
}

bool Path::close() noexcept {
  if (!isClosed() && (mVertices.count() > 1)) {
    addVertex(mVertices.first().getPos(), Angle::deg0());
    Q_ASSERT(isClosed());
    return true;
  } else {
    return false;
  }
}

void Path::serialize(SExpression& root) const {
  for (const Vertex& vertex : mVertices) {
    root.ensureLineBreak();
    vertex.serialize(root.appendList("vertex"));
  }
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

Path& Path::operator=(const Path& rhs) noexcept {
  mVertices = rhs.mVertices;
  mPainterPathPx = rhs.mPainterPathPx;
  return *this;
}

bool Path::operator<(const Path& rhs) const noexcept {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
  return mVertices < rhs.mVertices;
#else
  return std::lexicographical_compare(mVertices.begin(), mVertices.end(),
                                      rhs.mVertices.begin(),
                                      rhs.mVertices.end());
#endif
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

Path Path::line(const Point& p1, const Point& p2, const Angle& angle) noexcept {
  return Path({Vertex(p1, angle), Vertex(p2)});
}

Path Path::circle(const PositiveLength& diameter) noexcept {
  return obround(diameter, diameter);
}

Path Path::obround(const PositiveLength& width,
                   const PositiveLength& height) noexcept {
  Path p;
  Length rx = width / 2;
  Length ry = height / 2;
  if (width > height) {
    p.addVertex(Point(ry - rx, ry), Angle::deg0());
    p.addVertex(Point(rx - ry, ry), -Angle::deg180());
    p.addVertex(Point(rx - ry, -ry), Angle::deg0());
    p.addVertex(Point(ry - rx, -ry), -Angle::deg180());
    p.addVertex(Point(ry - rx, ry), Angle::deg0());
  } else if (width < height) {
    p.addVertex(Point(rx, ry - rx), Angle::deg0());
    p.addVertex(Point(rx, rx - ry), -Angle::deg180());
    p.addVertex(Point(-rx, rx - ry), Angle::deg0());
    p.addVertex(Point(-rx, ry - rx), -Angle::deg180());
    p.addVertex(Point(rx, ry - rx), Angle::deg0());
  } else {
    Q_ASSERT(width == height);
    p.addVertex(Point(rx, 0), -Angle::deg180());
    p.addVertex(Point(-rx, 0), -Angle::deg180());
    p.addVertex(Point(rx, 0), Angle::deg0());
  }
  return p;
}

Path Path::obround(const Point& p1, const Point& p2,
                   const PositiveLength& width) noexcept {
  Point diff = p2 - p1;
  Path p = obround(UnsignedLength(diff.getLength()) + width, width);
  p.rotate(Angle::fromRad(qAtan2(diff.getY().toMm(), diff.getX().toMm())));
  p.translate((p1 + p2) / 2);
  return p;
}

Path Path::arcObround(const Point& p1, const Point& p2, const Angle& angle,
                      const PositiveLength& width) noexcept {
  if (p1 == p2) {
    return circle(width).translated(p1);
  }
  Length radius = Toolbox::arcRadius(p1, p2, angle).abs();
  Length innerRadius = radius - (*width / 2);
  Length outerRadius = radius + (*width / 2);
  Point center = Toolbox::arcCenter(p1, p2, angle);
  Point delta1 = p1 - center;
  Point delta2 = p2 - center;
  qreal angle1Rad = qAtan2(delta1.getY().toPx(), delta1.getX().toPx());
  qreal angle2Rad = qAtan2(delta2.getY().toPx(), delta2.getX().toPx());
  Point p1Inner =
      center + Point(innerRadius, 0).rotated(Angle::fromRad(angle1Rad));
  Point p1Outer =
      center + Point(outerRadius, 0).rotated(Angle::fromRad(angle1Rad));
  Point p2Inner =
      center + Point(innerRadius, 0).rotated(Angle::fromRad(angle2Rad));
  Point p2Outer =
      center + Point(outerRadius, 0).rotated(Angle::fromRad(angle2Rad));

  Path p;
  p.addVertex(p1Inner, angle);
  p.addVertex(p2Inner, angle < 0 ? Angle::deg180() : -Angle::deg180());
  p.addVertex(p2Outer, -angle);
  p.addVertex(p1Outer, angle < 0 ? Angle::deg180() : -Angle::deg180());
  p.addVertex(p1Inner, Angle::deg0());
  return p;
}

Path Path::rect(const Point& p1, const Point& p2) noexcept {
  Path p;
  p.addVertex(Point(p1.getX(), p1.getY()));
  p.addVertex(Point(p2.getX(), p1.getY()));
  p.addVertex(Point(p2.getX(), p2.getY()));
  p.addVertex(Point(p1.getX(), p2.getY()));
  p.addVertex(Point(p1.getX(), p1.getY()));
  return p;
}

Path Path::centeredRect(const PositiveLength& width,
                        const PositiveLength& height) noexcept {
  Path p;
  Length rx = width / 2;
  Length ry = height / 2;
  p.addVertex(Point(-rx, ry));
  p.addVertex(Point(rx, ry));
  p.addVertex(Point(rx, -ry));
  p.addVertex(Point(-rx, -ry));
  p.addVertex(Point(-rx, ry));
  return p;
}

Path Path::octagon(const PositiveLength& width,
                   const PositiveLength& height) noexcept {
  Path p;
  Length rx = width / 2;
  Length ry = height / 2;
  Length a = Length::fromMm(qMin(rx, ry).toMm() * (2 - qSqrt(2)));
  p.addVertex(Point(rx, ry - a));
  p.addVertex(Point(rx - a, ry));
  p.addVertex(Point(a - rx, ry));
  p.addVertex(Point(-rx, ry - a));
  p.addVertex(Point(-rx, a - ry));
  p.addVertex(Point(a - rx, -ry));
  p.addVertex(Point(rx - a, -ry));
  p.addVertex(Point(rx, a - ry));
  p.addVertex(Point(rx, ry - a));
  return p;
}

Path Path::flatArc(const Point& p1, const Point& p2, const Angle& angle,
                   const PositiveLength& maxTolerance) noexcept {
  // return straight line if radius is smaller than half of the allowed
  // tolerance
  Length radiusAbs = Toolbox::arcRadius(p1, p2, angle).abs();
  if (radiusAbs <= maxTolerance / 2) {
    return line(p1, p2);
  }

  // calculate how many lines we need to create
  qreal radiusAbsNm = static_cast<qreal>(radiusAbs.toNm());
  qreal y = qBound(qreal(0.0), static_cast<qreal>(maxTolerance->toNm()),
                   radiusAbsNm / qreal(4));
  qreal stepsPerRad =
      qMin(qreal(0.5) / qAcos(1 - y / radiusAbsNm), radiusAbsNm / qreal(2));
  int steps = qCeil(stepsPerRad * angle.abs().toRad());

  // some other very complex calculations...
  qreal angleDelta = angle.toMicroDeg() / (qreal)steps;
  Point center = Toolbox::arcCenter(p1, p2, angle);

  // create line segments
  Path p;
  p.addVertex(p1);
  for (int i = 1; i < steps; ++i) {
    p.addVertex(p1.rotated(Angle(angleDelta * i), center));
  }
  p.addVertex(p2);
  return p;
}

QPainterPath Path::toQPainterPathPx(const QVector<Path>& paths,
                                    bool area) noexcept {
  QPainterPath p;
  p.setFillRule(Qt::WindingFill);
  foreach (const Path& path, paths) {
    if (area) {
      p |= path.toQPainterPathPx();
    } else {
      p.addPath(path.toQPainterPathPx());
    }
  }
  return p;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
