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

#include <QtCore>

#include <limits>
#include <type_traits>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Conversions
 ******************************************************************************/

QString Length::toMmString() const noexcept {
  using LengthBaseU_t = std::make_unsigned<LengthBase_t>::type;

  LengthBase_t nm = toNm();
  LengthBaseU_t nm_abs;
  if (nm < 0) {
    nm_abs = -static_cast<LengthBaseU_t>(nm);
  } else {
    nm_abs = static_cast<LengthBaseU_t>(nm);
  }

  QString str = QString::number(nm_abs);
  if (nm_abs >= 1000000) {
    str.insert(str.length() - 6, '.');
    while (str.endsWith('0') && !str.endsWith(".0"))
      str.chop(1);
  } else if (nm_abs == 0) {
      str.append(".0");
  } else {
    while (str.endsWith('0'))
        str.chop(1);

    if (nm_abs >= 100000) {
      // number is 0.X, X non zero
      str.insert(0, "0.");
    } else if (nm_abs >= 10000) {
      // number is 0.0X..., X non zero
      str.insert(0, "0.0");
    } else if (nm_abs >= 1000) {
      // number is 0.00X..., X non zero
      str.insert(0, "0.00");
    } else if (nm_abs >= 100) {
      // number is 0.000X..., X non zero
      str.insert(0, "0.000");
    } else if (nm_abs >= 10) {
      // number is 0.0000X..., X non zero
      str.insert(0, "0.0000");
    } else /*if (nm_abs >= 1)*/ {
      // number is 0.00000X, X non zero
      str.insert(0, "0.00000");
    }
  }

  if (nm < 0)
    str.insert(0, '-');

  return str;
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

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Length::setLengthFromFloat(qreal nanometers) {
  LengthBase_t min   = std::numeric_limits<LengthBase_t>::min();
  LengthBase_t max   = std::numeric_limits<LengthBase_t>::max();
  qreal        value = qRound(nanometers);
  if ((value > max) || (value < min)) {
    throw RangeError(__FILE__, __LINE__, value, min, max);
  }

  mNanometers = value;
}

/*******************************************************************************
 *  Private Static Methods
 ******************************************************************************/

LengthBase_t Length::mapNmToGrid(LengthBase_t  nanometers,
                                 const Length& gridInterval) noexcept {
  using LengthBaseU_t = std::make_unsigned<LengthBase_t>::type;

  LengthBaseU_t grid_interval = static_cast<LengthBaseU_t>(gridInterval.abs().mNanometers);
  if (grid_interval == 0)
    return nanometers;

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
  if (remainder >= (grid_interval / 2)) {
    // snap away from zero, but it might overflow
    LengthBaseU_t tmp_snapped = nm_abs + (grid_interval - remainder);
    if (tmp_snapped < nm_abs || tmp_snapped > max) {
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
  using LengthBaseU_t = std::make_unsigned<LengthBase_t>::type;

  const LengthBase_t  min   = std::numeric_limits<LengthBase_t>::min();
  const LengthBase_t  max   = std::numeric_limits<LengthBase_t>::max();
  const LengthBaseU_t max_u = std::numeric_limits<LengthBaseU_t>::max();

  enum class State {
    INVALID,
    START,
    AFTER_SIGN,
    LONELY_DOT,
    INT_PART,
    FRAC_PART,
    EXP,
    EXP_AFTER_SIGN,
    EXP_DIGITS,
  };
  State state = State::START;
  LengthBaseU_t value_abs = 0;
  bool sign = false;
  qint32 exp_offset = 6;

  const quint32 max_exp = std::numeric_limits<quint32>::max();
  quint32 exp = 0;
  bool exp_sign = false;

  for (QChar c : millimeters) {
    if (state == State::INVALID) {
      // break the loop, not the switch
      break;
    }
    switch (state) {
    case State::INVALID:
      // already checked, but needed to avoid compiler warnings
      break;

    case State::START:
      if (c == '-') {
        sign = true;
        state = State::AFTER_SIGN;
      } else if (c == '+') {
        state = State::AFTER_SIGN;
      } else if (c == '.') {
        state = State::LONELY_DOT;
      } else if (c.isDigit()) {
          value_abs = static_cast<LengthBaseU_t>(c.digitValue());
          state = State::INT_PART;
      } else {
        state = State::INVALID;
      }
      break;

    case State::AFTER_SIGN:
      if (c == '.') {
        state = State::LONELY_DOT;
      } else if (c.isDigit()) {
          value_abs = static_cast<LengthBaseU_t>(c.digitValue());
          state = State::INT_PART;
      } else {
        state = State::INVALID;
      }
      break;

    case State::LONELY_DOT:
      if (c.isDigit()) {
          value_abs = static_cast<LengthBaseU_t>(c.digitValue());
          exp_offset -= 1;
          state = State::FRAC_PART;
      } else {
        state = State::INVALID;
      }
      break;

    case State::INT_PART:
      if (c == '.') {
        state = State::FRAC_PART;
      } else if (c == 'e' || c == 'E') {
        state = State::EXP;
      } else if (c.isDigit()) {
          LengthBaseU_t digit = static_cast<LengthBaseU_t>(c.digitValue());
          if (value_abs > (max_u / 10)) {
            // Would overflow
            state = State::INVALID;
            break;
          }
          value_abs *= 10;
          if (value_abs > (max_u - digit)) {
            // Would overflow
            state = State::INVALID;
            break;
          }
          value_abs += digit;
      } else {
        state = State::INVALID;
      }
      break;

    case State::FRAC_PART:
      if (c == 'e' || c == 'E') {
        state = State::EXP;
      } else if (c.isDigit()) {
          LengthBaseU_t digit = static_cast<LengthBaseU_t>(c.digitValue());
          if (value_abs > (max_u / 10)) {
            // Would overflow
            state = State::INVALID;
            break;
          }
          value_abs *= 10;
          if (value_abs > (max_u - digit)) {
            // Would overflow
            state = State::INVALID;
            break;
          }
          value_abs += digit;
          exp_offset -= 1;
      } else {
        state = State::INVALID;
      }
      break;

    case State::EXP:
      if (c == '-') {
        exp_sign = true;
        state = State::EXP_AFTER_SIGN;
      } else if (c == '+') {
        state = State::EXP_AFTER_SIGN;
      } else if (c.isDigit()) {
        exp = static_cast<quint32>(c.digitValue());
        state = State::EXP_DIGITS;
      } else {
        state = State::INVALID;
      }
      break;

    case State::EXP_AFTER_SIGN:
      if (c.isDigit()) {
        exp = static_cast<quint32>(c.digitValue());
        state = State::EXP_DIGITS;
      } else {
        state = State::INVALID;
      }
      break;

    case State::EXP_DIGITS:
      if (c.isDigit()) {
        quint32 digit = static_cast<quint32>(c.digitValue());
        if (exp > (max_exp / 10)) {
            // Would overflow
            state = State::INVALID;
            break;
          }
          exp *= 10;
          if (exp > (max_exp - digit)) {
            // Would overflow
            state = State::INVALID;
            break;
          }
          exp += digit;
      } else {
        state = State::INVALID;
      }
    }
  }

  bool ok = true;
  switch (state) {
  case State::INVALID:
  case State::START:
  case State::AFTER_SIGN:
  case State::LONELY_DOT:
  case State::EXP:
  case State::EXP_AFTER_SIGN:
    ok = false;
    break;

  case State::INT_PART:
  case State::FRAC_PART:
  case State::EXP_DIGITS:
    break;
  }

  if (ok) {
    quint32 exp_offset_abs;
    if (exp_offset < 0) {
      exp_offset_abs = -static_cast<quint32>(exp_offset);
    } else {
      exp_offset_abs = static_cast<quint32>(exp_offset);
    }

    if (exp_sign == (exp_offset < 0)) {
      if (exp > (max_exp - exp_offset_abs)) {
        // would overflow
        ok = false;
      } else {
        exp += exp_offset_abs;
      }
    } else {
      if (exp < exp_offset_abs) {
        // would overflow
        ok = false;
      } else {
        exp -= exp_offset_abs;
      }
    }
  }

  LengthBase_t nm = 0;
  if (ok) {
    // No need to apply exponent or sign if value_abs is zero
    if (value_abs != 0) {
      if (exp_sign) {
        for (quint32 i = 0; i < exp; i++) {
          if ((value_abs % 10) != 0) {
            // sub-nanometer precision
            ok = false;
            break;
          }
          value_abs /= 10;
        }
      } else {
        for (quint32 i = 0; i < exp; i++) {
          if (value_abs > (max_u / 10)) {
            // would overflow
            ok = false;
            break;
          }
          value_abs *= 10;
        }
      }
      if (ok) {
        if (sign) {
          if (value_abs > static_cast<LengthBaseU_t>(min)) {
            ok = false;
          } else {
            nm = static_cast<LengthBase_t>(-value_abs);
          }
        } else {
          if (value_abs > static_cast<LengthBaseU_t>(max)) {
            ok = false;
          } else {
            nm = static_cast<LengthBase_t>(value_abs);
          }
        }
      }
    }
  }

  if (!ok) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("Invalid length string: \"%1\"")).arg(millimeters));
  }
  return nm;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
