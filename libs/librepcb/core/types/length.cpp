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
#include "length.h"

#include "../serialization/sexpression.h"
#include "../utils/qtmetatyperegistration.h"
#include "../utils/toolbox.h"

#include <librepcb/rust-core/ffi.h>
#include <type_traits>

#include <QtCore>

#include <limits>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

// Register Qt meta types.
static QtMetaTypeRegistration<Length> sMetaType;

/*******************************************************************************
 *  Conversions
 ******************************************************************************/

QString Length::toMmString() const noexcept {
  return Toolbox::decimalFixedPointToString<int64_t>(toNm(), 6);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

Length Length::abs() const noexcept {
  return Length(rs::ffi_length_abs(mNanometers));
}

Length Length::mappedToGrid(const Length& gridInterval) const noexcept {
  Length length(*this);
  return length.mapToGrid(gridInterval);
}

Length& Length::mapToGrid(const Length& gridInterval) noexcept {
  if (gridInterval != 0) {
    mNanometers =
        rs::ffi_length_rounded_to(mNanometers, gridInterval.abs().toNm());
  }
  return *this;
}

Length Length::roundedDownTo(const Length& multiple) const noexcept {
  if (multiple != 0) {
    return Length(
        rs::ffi_length_rounded_down_to(mNanometers, multiple.abs().toNm()));
  } else {
    return *this;
  }
}

Length Length::roundedUpTo(const Length& multiple) const noexcept {
  if (multiple != 0) {
    return Length(
        rs::ffi_length_rounded_up_to(mNanometers, multiple.abs().toNm()));
  } else {
    return *this;
  }
}

Length Length::scaled(qreal factor) const noexcept {
  int64_t nm = 0;
  const bool ok = rs::ffi_length_from_nm_f(mNanometers * factor, &nm);
  Q_ASSERT(ok);
  return Length(nm);
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

std::optional<Length> Length::tryFromNm(qreal nanometers) noexcept {
  int64_t nm;
  if (rs::ffi_length_from_nm_f(nanometers, &nm)) {
    return Length(nm);
  }
  return std::nullopt;
}

Length Length::fromNm(qreal nanometers) {
  int64_t nm;
  if (rs::ffi_length_from_nm_f(nanometers, &nm)) {
    return Length(nm);
  }
  throw RangeError(__FILE__, __LINE__, nanometers,
                   std::numeric_limits<int64_t>::min(),
                   std::numeric_limits<int64_t>::max());
}

Length Length::fromMm(qreal millimeters) {
  return fromNm(millimeters * 1e6);
}

Length Length::fromMm(const QString& millimeters) {
  return Length(Toolbox::decimalFixedPointFromString<int64_t>(millimeters, 6));
}

Length Length::fromInch(qreal inches) {
  return fromNm(inches * sNmPerInch);
}

Length Length::fromMil(qreal mils) {
  return fromNm(mils * sNmPerMil);
}

Length Length::fromPx(qreal pixels) {
  return fromNm(pixels * sNmPerPixel);
}

Length Length::min() noexcept {
  return Length(std::numeric_limits<int64_t>::min());
}

Length Length::max() noexcept {
  return Length(std::numeric_limits<int64_t>::max());
}

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
std::unique_ptr<SExpression> serialize(const Length& obj) {
  return SExpression::createToken(obj.toMmString());
}

template <>
std::unique_ptr<SExpression> serialize(const UnsignedLength& obj) {
  return serialize(*obj);
}

template <>
std::unique_ptr<SExpression> serialize(const PositiveLength& obj) {
  return serialize(*obj);
}

template <>
Length deserialize(const SExpression& node) {
  return Length::fromMm(node.getValue());
}

template <>
UnsignedLength deserialize(const SExpression& node) {
  return UnsignedLength(deserialize<Length>(node));  // can throw
}

template <>
PositiveLength deserialize(const SExpression& node) {
  return PositiveLength(deserialize<Length>(node));  // can throw
}

template <>
Point3D deserialize(const SExpression& node) {
  return std::make_tuple(deserialize<Length>(node.getChild("@0")),
                         deserialize<Length>(node.getChild("@1")),
                         deserialize<Length>(node.getChild("@2")));
}

QDebug operator<<(QDebug stream, const Point3D& obj) {
  stream << QString("Point3D(%1mm, %2mm, %3mm)")
                .arg(std::get<0>(obj).toMm(), std::get<1>(obj).toMm(),
                     std::get<2>(obj).toMm());
  return stream;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
