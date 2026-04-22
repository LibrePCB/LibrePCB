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
#include "angle.h"

#include "../serialization/sexpression.h"
#include "../utils/qtmetatyperegistration.h"
#include "../utils/toolbox.h"

#include <librepcb/rust-core/ffi.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

// Register Qt meta type.
static QtMetaTypeRegistration<Angle> sMetaType;

/*******************************************************************************
 *  Class Angle
 ******************************************************************************/

// General Methods

QString Angle::toDegString() const noexcept {
  return Toolbox::decimalFixedPointToString<qint32>(toMicroDeg(), 6);
}

Angle Angle::abs() const noexcept {
  return Angle(rs::ffi_angle_abs(mMicrodegrees));
}

Angle Angle::inverted() const noexcept {
  return Angle(rs::ffi_angle_inverted(mMicrodegrees));
}

Angle Angle::rounded(const Angle& interval) const noexcept {
  if (interval > 0) {
    return Angle(
        rs::ffi_angle_rounded_to(mMicrodegrees, interval.toMicroDeg()));
  }
  qCritical() << "Invalid value passed to Angle::round():" << interval;
  return *this;
}

Angle Angle::mappedTo0_360deg() const noexcept {
  return Angle(rs::ffi_angle_to_0_360_deg(mMicrodegrees));
}

Angle Angle::mappedTo180deg() const noexcept {
  return Angle(rs::ffi_angle_to_180_deg(mMicrodegrees));
}

// Static Methods

Angle Angle::fromDeg(qreal degrees) {
  int64_t udeg;
  if (rs::ffi_angle_from_deg_f(degrees, &udeg)) {
    return Angle(udeg);
  }
  // Value was outside the range of int64_t, or an invalid float.
  throw RangeError(__FILE__, __LINE__, degrees * 1e6,
                   std::numeric_limits<int64_t>::min(),
                   std::numeric_limits<int64_t>::max());
}

Angle Angle::fromDeg(const QString& degrees) {
  return Angle(degStringToMicrodeg(degrees));
}

Angle Angle::fromRad(qreal radians) {
  if (auto angle = tryFromRad(radians)) {
    return *angle;
  }
  // Value was outside the range of int64_t, or an invalid float.
  throw RangeError(__FILE__, __LINE__, radians * (180.0 / M_PI) * 1e6,
                   std::numeric_limits<int64_t>::min(),
                   std::numeric_limits<int64_t>::max());
}

std::optional<Angle> Angle::tryFromRad(qreal radians) noexcept {
  int64_t udeg;
  if (rs::ffi_angle_from_rad_f(radians, &udeg)) {
    return Angle(udeg);
  } else {
    return std::nullopt;
  }
}

// Private Static Methods

qint32 Angle::degStringToMicrodeg(const QString& degrees) {
  return Toolbox::decimalFixedPointFromString<qint32>(degrees, 6);
}

// Non-Member Functions

template <>
std::unique_ptr<SExpression> serialize(const Angle& obj) {
  return SExpression::createToken(obj.toDegString());
}

template <>
Angle deserialize(const SExpression& node) {
  return Angle::fromDeg(node.getValue());  // can throw
}

template <>
Angle3D deserialize(const SExpression& node) {
  return std::make_tuple(deserialize<Angle>(node.getChild("@0")),
                         deserialize<Angle>(node.getChild("@1")),
                         deserialize<Angle>(node.getChild("@2")));
}

QDebug operator<<(QDebug stream, const Angle3D& obj) {
  stream << QString("Angle3D(%1°, %2°, %3°)")
                .arg(std::get<0>(obj).toDeg(), std::get<1>(obj).toDeg(),
                     std::get<2>(obj).toDeg());
  return stream;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
