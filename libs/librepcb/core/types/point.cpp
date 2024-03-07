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
#include "point.h"

#include "../serialization/sexpression.h"
#include "../utils/qtmetatyperegistration.h"
#include "angle.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

// Register Qt meta type.
static QtMetaTypeRegistration<Point> sMetaType;

/*******************************************************************************
 *  Class Point
 ******************************************************************************/

Point::Point(const SExpression& node)
  : mX(deserialize<Length>(node.getChild("@0"))),
    mY(deserialize<Length>(node.getChild("@1"))) {
}

// General Methods

Point Point::abs() const noexcept {
  Point p(*this);
  p.makeAbs();
  return p;
}

Point& Point::makeAbs() noexcept {
  mX.makeAbs();
  mY.makeAbs();
  return *this;
}

Point Point::mappedToGrid(const PositiveLength& gridInterval) const noexcept {
  Point p(*this);
  p.mapToGrid(gridInterval);
  return p;
}

Point& Point::mapToGrid(const PositiveLength& gridInterval) noexcept {
  mX.mapToGrid(*gridInterval);
  mY.mapToGrid(*gridInterval);
  return *this;
}

bool Point::isOnGrid(const PositiveLength& gridInterval) const noexcept {
  return (mappedToGrid(gridInterval) == *this);
}

Point Point::rotated(const Angle& angle, const Point& center) const noexcept {
  Point p(*this);
  p.rotate(angle, center);
  return p;
}

Point& Point::rotate(const Angle& angle, const Point& center) noexcept {
  Length dx = mX - center.getX();
  Length dy = mY - center.getY();
  Angle angle0_360 = angle.mappedTo0_360deg();

  // if angle is a multiple of 90 degrees, rotating can be done without losing
  // accuracy
  if (angle0_360 == Angle::deg90()) {
    setX(center.getX() - dy);
    setY(center.getY() + dx);
  } else if (angle0_360 == Angle::deg180()) {
    setX(center.getX() - dx);
    setY(center.getY() - dy);
  } else if (angle0_360 == Angle::deg270()) {
    setX(center.getX() + dy);
    setY(center.getY() - dx);
  } else if (angle != Angle::deg0()) {
    // angle is not a multiple of 90 degrees --> we must use floating point
    // arithmetic
    qreal sin = std::sin(angle.toRad());
    qreal cos = std::cos(angle.toRad());
    setX(Length::fromMm(center.getX().toMm() + cos * dx.toMm() -
                        sin * dy.toMm()));
    setY(Length::fromMm(center.getY().toMm() + sin * dx.toMm() +
                        cos * dy.toMm()));
  }  // else: angle == 0°, nothing to do...

  return *this;
}

Point Point::mirrored(Qt::Orientation orientation,
                      const Point& center) const noexcept {
  Point p(*this);
  p.mirror(orientation, center);
  return p;
}

Point& Point::mirror(Qt::Orientation orientation,
                     const Point& center) noexcept {
  switch (orientation) {
    case Qt::Horizontal:
      mX += Length(2) * (center.getX() - mX);
      break;
    case Qt::Vertical:
      mY += Length(2) * (center.getY() - mY);
      break;
    default:
      Q_ASSERT(false);
  }
  return *this;
}

void Point::serialize(SExpression& root) const {
  root.appendChild(mX);
  root.appendChild(mY);
}

// Static Methods

Point Point::fromMm(qreal millimetersX, qreal millimetersY) {
  Point p;
  p.mX.setLengthMm(millimetersX);
  p.mY.setLengthMm(millimetersY);
  return p;
}

Point Point::fromMm(const QPointF& millimeters) {
  return fromMm(millimeters.x(), millimeters.y());
}

Point Point::fromInch(qreal inchesX, qreal inchesY) {
  Point p;
  p.mX.setLengthInch(inchesX);
  p.mY.setLengthInch(inchesY);
  return p;
}

Point Point::fromInch(const QPointF& inches) {
  return fromInch(inches.x(), inches.y());
}

Point Point::fromMil(qreal milsX, qreal milsY) {
  Point p;
  p.mX.setLengthMil(milsX);
  p.mY.setLengthMil(milsY);
  return p;
}

Point Point::fromMil(const QPointF& mils) {
  return fromMil(mils.x(), mils.y());
}

Point Point::fromPx(qreal pixelsX, qreal pixelsY) {
  Point p;
  p.mX.setLengthPx(pixelsX);
  p.mY.setLengthPx(-pixelsY);  // invert Y!
  return p;
}

Point Point::fromPx(const QPointF& pixels) {
  return fromPx(pixels.x(), pixels.y());
}

// Non-Member Functions

QDataStream& operator<<(QDataStream& stream, const Point& point) {
  stream << point.toMmQPointF();
  return stream;
}

QDebug operator<<(QDebug stream, const Point& point) {
  stream << QString("Point(%1mm, %2mm)")
                .arg(point.toMmQPointF().x())
                .arg(point.toMmQPointF().y());
  return stream;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
