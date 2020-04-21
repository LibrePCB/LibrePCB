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

#include "../toolbox.h"

#include <type_traits>

#include <QtCore>

#include <limits>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Conversions
 ******************************************************************************/

QString Length::toMmString() const noexcept {
  return Toolbox::decimalFixedPointToString<LengthBase_t>(toNm(), 6);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

Length Length::abs() const noexcept {
  Length l(*this);
  l.makeAbs();
  return l;
}

Length& Length::makeAbs() noexcept {
  if (mNanometers == std::numeric_limits<LengthBase_t>::min()) {
    mNanometers = std::numeric_limits<LengthBase_t>::max();
  } else {
    mNanometers = qAbs(mNanometers);
  }
  return *this;
}

Length Length::mappedToGrid(const Length& gridInterval) const noexcept {
  Length length(*this);
  return length.mapToGrid(gridInterval);
}

Length& Length::mapToGrid(const Length& gridInterval) noexcept {
  mNanometers = mapNmToGrid(mNanometers, gridInterval);
  return *this;
}

Length Length::scaled(qreal factor) const noexcept {
  Length length(*this);
  return length.scale(factor);
}

Length& Length::scale(qreal factor) noexcept {
  mNanometers *= factor;
  return *this;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

Length Length::fromMm(qreal millimeters, const Length& gridInterval) {
  Length l;
  l.setLengthMm(millimeters);
  return l.mapToGrid(gridInterval);
}

Length Length::fromMm(const QString& millimeters, const Length& gridInterval) {
  Length l;
  l.setLengthMm(millimeters);
  return l.mapToGrid(gridInterval);
}

Length Length::fromInch(qreal inches, const Length& gridInterval) {
  Length l;
  l.setLengthInch(inches);
  return l.mapToGrid(gridInterval);
}

Length Length::fromMil(qreal mils, const Length& gridInterval) {
  Length l;
  l.setLengthMil(mils);
  return l.mapToGrid(gridInterval);
}

Length Length::fromPx(qreal pixels, const Length& gridInterval) {
  Length l;
  l.setLengthPx(pixels);
  return l.mapToGrid(gridInterval);
}

Length Length::min() noexcept {
  return Length(std::numeric_limits<LengthBase_t>::min());
}

Length Length::max() noexcept {
  return Length(std::numeric_limits<LengthBase_t>::max());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Length::setLengthFromFloat(qreal nanometers) {
  LengthBase_t min = std::numeric_limits<LengthBase_t>::min();
  LengthBase_t max = std::numeric_limits<LengthBase_t>::max();
  if ((nanometers > static_cast<qreal>(max)) ||
      (nanometers < static_cast<qreal>(min))) {
    throw RangeError(__FILE__, __LINE__, nanometers, min, max);
  }

  mNanometers = qRound64(nanometers);
}

/*******************************************************************************
 *  Private Static Methods
 ******************************************************************************/

LengthBase_t Length::mapNmToGrid(LengthBase_t  nanometers,
                                 const Length& gridInterval) noexcept {
  using LengthBaseU_t = std::make_unsigned<LengthBase_t>::type;

  LengthBaseU_t grid_interval =
      static_cast<LengthBaseU_t>(gridInterval.abs().mNanometers);
  if (grid_interval == 0) return nanometers;

  LengthBaseU_t nm_abs;
  LengthBaseU_t max;

  if (nanometers >= 0) {
    nm_abs = static_cast<LengthBaseU_t>(nanometers);
    max = static_cast<LengthBaseU_t>(std::numeric_limits<LengthBase_t>::max());
  } else {
    nm_abs = -static_cast<LengthBaseU_t>(nanometers);
    max = static_cast<LengthBaseU_t>(std::numeric_limits<LengthBase_t>::min());
  }

  LengthBaseU_t remainder = nm_abs % grid_interval;
  if (remainder > (grid_interval / 2)) {
    // snap away from zero, but it might overflow
    LengthBaseU_t tmp_snapped = nm_abs + (grid_interval - remainder);
    if ((tmp_snapped < nm_abs) || (tmp_snapped > max)) {
      // overflow, snap towards zero
      nm_abs -= remainder;
    } else {
      nm_abs = tmp_snapped;
    }
  } else {
    // snap towards zero
    nm_abs -= remainder;
  }

  if (nanometers >= 0)
    return static_cast<LengthBase_t>(nm_abs);
  else
    return static_cast<LengthBase_t>(-nm_abs);
}

LengthBase_t Length::mmStringToNm(const QString& millimeters) {
  return Toolbox::decimalFixedPointFromString<LengthBase_t>(millimeters, 6);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
