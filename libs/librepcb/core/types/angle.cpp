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

#include "../utils/toolbox.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Angle
 ******************************************************************************/

// General Methods

QString Angle::toDegString() const noexcept {
  return Toolbox::decimalFixedPointToString<qint32>(toMicroDeg(), 6);
}

Angle Angle::abs() const noexcept {
  Angle a(*this);
  a.makeAbs();
  return a;
}

Angle& Angle::makeAbs() noexcept {
  mMicrodegrees = qAbs(mMicrodegrees);
  return *this;
}

Angle Angle::inverted() const noexcept {
  Angle a(*this);
  a.invert();
  return a;
}

Angle& Angle::invert() noexcept {
  if (mMicrodegrees > 0) {
    mMicrodegrees -= 360000000;
  } else if (mMicrodegrees < 0) {
    mMicrodegrees += 360000000;
  }
  return *this;
}

Angle Angle::rounded(const Angle& interval) const noexcept {
  Angle a(*this);
  a.round(interval);
  return a;
}

Angle& Angle::round(const Angle& interval) noexcept {
  if (interval > 0) {
    qint32 value = mMicrodegrees;
    value += (value >= 0) ? (interval.mMicrodegrees / 2)
                          : (-interval.mMicrodegrees / 2);
    setAngleMicroDeg(interval.mMicrodegrees * (value / interval.mMicrodegrees));
  } else {
    qCritical() << "Invalid value passed to Angle::round():" << interval;
  }
  return *this;
}

Angle Angle::mappedTo0_360deg() const noexcept {
  Angle a(*this);
  a.mapTo0_360deg();
  return a;
}

Angle& Angle::mapTo0_360deg() noexcept {
  if (mMicrodegrees < 0) mMicrodegrees += 360000000;
  return *this;
}

Angle Angle::mappedTo180deg() const noexcept {
  Angle a(*this);
  a.mapTo180deg();
  return a;
}

Angle& Angle::mapTo180deg() noexcept {
  if (mMicrodegrees < -180000000)
    mMicrodegrees += 360000000;
  else if (mMicrodegrees >= 180000000)
    mMicrodegrees -= 360000000;
  return *this;
}

// Static Methods

Angle Angle::fromDeg(qreal degrees) noexcept {
  Angle angle;
  angle.setAngleDeg(degrees);
  return angle;
}

Angle Angle::fromDeg(const QString& degrees) {
  Angle angle;
  angle.setAngleDeg(degrees);
  return angle;
}

Angle Angle::fromRad(qreal radians) noexcept {
  Angle angle;
  angle.setAngleRad(radians);
  return angle;
}

// Private Static Methods

qint32 Angle::degStringToMicrodeg(const QString& degrees) {
  return Toolbox::decimalFixedPointFromString<qint32>(degrees, 6);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
