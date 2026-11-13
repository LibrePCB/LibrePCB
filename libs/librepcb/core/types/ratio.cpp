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
#include "ratio.h"

#include "../serialization/serialization.h"
#include "../serialization/sexpression.h"
#include "../utils/toolbox.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Ratio
 ******************************************************************************/

QString Ratio::toNormalizedString() const noexcept {
  return Toolbox::decimalFixedPointToString<qint32>(toPpm(), 6);
}

// Static Methods

Ratio Ratio::fromPercent(int percent) noexcept {
  Ratio ratio;
  ratio.setRatioPercent(percent);
  return ratio;
}

Ratio Ratio::fromPercent(qreal percent) noexcept {
  Ratio ratio;
  ratio.setRatioPercent(percent);
  return ratio;
}

Ratio Ratio::fromNormalized(qreal normalized) noexcept {
  Ratio ratio;
  ratio.setRatioNormalized(normalized);
  return ratio;
}

Ratio Ratio::fromNormalized(const QString& normalized) {
  Ratio ratio;
  ratio.setRatioNormalized(normalized);
  return ratio;
}

std::optional<Ratio> Ratio::tryFromNormalized(
    const QString& normalized) noexcept {
  // Not a nice implementation, but will be obsolete after Rust migration.
  try {
    return fromNormalized(normalized);
  } catch (const Exception& e) {
    return std::nullopt;
  }
}

// Private Static Methods

qint32 Ratio::normalizedStringToPpm(const QString& normalized) {
  return Toolbox::decimalFixedPointFromString<qint32>(normalized, 6);
}

// Non-Member Functions

template <>
std::unique_ptr<SExpression> serialize(const Ratio& obj) {
  return SExpression::createToken(obj.toNormalizedString());
}

template <>
std::unique_ptr<SExpression> serialize(const UnsignedRatio& obj) {
  return serialize(*obj);
}

template <>
std::unique_ptr<SExpression> serialize(const UnsignedLimitedRatio& obj) {
  return serialize(*obj);
}

template <>
Ratio deserialize(const SExpression& node) {
  return Ratio::fromNormalized(node.getValue());
}

template <>
UnsignedRatio deserialize(const SExpression& node) {
  return UnsignedRatio(deserialize<Ratio>(node));  // can throw
}

template <>
UnsignedLimitedRatio deserialize(const SExpression& node) {
  return UnsignedLimitedRatio(deserialize<Ratio>(node));  // can throw
}

template <>
QString serializeToString(const Ratio& obj) noexcept {
  return obj.toNormalizedString();
}

template <>
QString serializeToString(const UnsignedRatio& obj) noexcept {
  return serializeToString(*obj);
}

template <>
QString serializeToString(const UnsignedLimitedRatio& obj) noexcept {
  return serializeToString(*obj);
}

template <>
std::optional<Ratio> deserialize(const QString& str) noexcept {
  return Ratio::tryFromNormalized(str);
}

template <>
std::optional<UnsignedRatio> deserialize(const QString& str) noexcept {
  if (auto r = deserialize<Ratio>(str)) {
    if (UnsignedRatioConstraint()(*r)) {
      return UnsignedRatio(*r);
    }
  }
  return std::nullopt;
}

template <>
std::optional<UnsignedLimitedRatio> deserialize(const QString& str) noexcept {
  if (auto r = deserialize<Ratio>(str)) {
    if (UnsignedLimitedRatioConstraint()(*r)) {
      return UnsignedLimitedRatio(*r);
    }
  }
  return std::nullopt;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
