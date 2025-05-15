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
#include "lengthunit.h"

#include "../serialization/sexpression.h"
#include "../utils/toolbox.h"
#include "length.h"
#include "point.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class LengthUnit
 ******************************************************************************/

// General Methods

QString LengthUnit::toStr() const noexcept {
  switch (mUnit) {
    case LengthUnit_t::Millimeters:
      return QString("millimeters");
    case LengthUnit_t::Micrometers:
      return QString("micrometers");
    case LengthUnit_t::Nanometers:
      return QString("nanometers");
    case LengthUnit_t::Inches:
      return QString("inches");
    case LengthUnit_t::Mils:
      return QString("mils");
    default:
      qCritical() << "Unhandled switch-case in LengthUnit::toStr():"
                  << static_cast<int>(mUnit);
      Q_ASSERT(false);
      return QString();
  }
}

QString LengthUnit::toStringTr() const noexcept {
  switch (mUnit) {
    case LengthUnit_t::Millimeters:
      return tr("Millimeters");
    case LengthUnit_t::Micrometers:
      return tr("Micrometers");
    case LengthUnit_t::Nanometers:
      return tr("Nanometers");
    case LengthUnit_t::Inches:
      return tr("Inches");
    case LengthUnit_t::Mils:
      return tr("Mils");
    default:
      qCritical() << "Unhandled switch-case in LengthUnit::toStringTr():"
                  << static_cast<int>(mUnit);
      Q_ASSERT(false);
      return QString();
  }
}

QString LengthUnit::toShortStringTr() const noexcept {
  switch (mUnit) {
    case LengthUnit_t::Millimeters:
      return "mm";
    case LengthUnit_t::Micrometers:
      return "μm";
    case LengthUnit_t::Nanometers:
      return "nm";
    case LengthUnit_t::Inches:
      return "″";
    case LengthUnit_t::Mils:
      return "mils";
    default:
      qCritical() << "Unhandled switch-case in LengthUnit::toShortStringTr():"
                  << static_cast<int>(mUnit);
      Q_ASSERT(false);
      return QString();
  }
}

QString LengthUnit::format(const Length& value, const QLocale& locale,
                           const QString& separator) const noexcept {
  return Toolbox::floatToString(convertToUnit(value),
                                getReasonableNumberOfDecimals(), locale) %
      separator % toShortStringTr();
}

int LengthUnit::getReasonableNumberOfDecimals() const noexcept {
  switch (mUnit) {
    case LengthUnit_t::Millimeters:
      return 3;
    case LengthUnit_t::Micrometers:
      return 1;
    case LengthUnit_t::Nanometers:
      return 0;
    case LengthUnit_t::Inches:
      return 5;
    case LengthUnit_t::Mils:
      return 2;
    default:
      qCritical() << "Unhandled switch-case in "
                     "LengthUnit::getReasonableNumberOfDecimals():"
                  << static_cast<int>(mUnit);
      Q_ASSERT(false);
      return 3;
  }
}

QStringList LengthUnit::getUserInputSuffixes() const noexcept {
  switch (mUnit) {
    case LengthUnit_t::Millimeters:
      return QStringList{"mm"};
    case LengthUnit_t::Micrometers:
      return QStringList{"μm", "um"};
    case LengthUnit_t::Nanometers:
      return QStringList{"nm"};
    case LengthUnit_t::Inches:
      return QStringList{"″", "\"", "in", "inch", "inches"};
    case LengthUnit_t::Mils:
      return QStringList{"mils"};
    default:
      qCritical()
          << "Unhandled switch-case in LengthUnit::getUserInputSuffixes():"
          << static_cast<int>(mUnit);
      Q_ASSERT(false);
      return QStringList();
  }
}

qreal LengthUnit::convertToUnit(const Length& length) const noexcept {
  switch (mUnit) {
    case LengthUnit_t::Millimeters:
      return length.toMm();
    case LengthUnit_t::Micrometers:
      return length.toMm() * (qreal)1000;
    case LengthUnit_t::Nanometers:
      return (qreal)length.toNm();
    case LengthUnit_t::Inches:
      return length.toInch();
    case LengthUnit_t::Mils:
      return length.toMil();
    default:
      qCritical() << "Unhandled switch-case in LengthUnit::convertToUnit():"
                  << static_cast<int>(mUnit);
      Q_ASSERT(false);
      return 0;
  }
}

QPointF LengthUnit::convertToUnit(const Point& point) const noexcept {
  switch (mUnit) {
    case LengthUnit_t::Millimeters:
      return point.toMmQPointF();
    case LengthUnit_t::Micrometers:
      return point.toMmQPointF() * (qreal)1000;
    case LengthUnit_t::Nanometers:
      return point.toMmQPointF() * (qreal)1000000;
    case LengthUnit_t::Inches:
      return point.toInchQPointF();
    case LengthUnit_t::Mils:
      return point.toMilQPointF();
    default:
      qCritical() << "Unhandled switch-case in LengthUnit::convertToUnit():"
                  << static_cast<int>(mUnit);
      Q_ASSERT(false);
      return QPointF();
  }
}

Length LengthUnit::convertFromUnit(qreal length) const {
  switch (mUnit) {
    case LengthUnit_t::Millimeters:
      return Length::fromMm(length);
    case LengthUnit_t::Micrometers:
      return Length::fromMm(length / (qreal)1000);
    case LengthUnit_t::Nanometers:
      return Length::fromMm(length / (qreal)1000000);
    case LengthUnit_t::Inches:
      return Length::fromInch(length);
    case LengthUnit_t::Mils:
      return Length::fromMil(length);
    default:
      qCritical() << "Unhandled switch-case in LengthUnit::convertFromUnit():"
                  << static_cast<int>(mUnit);
      Q_ASSERT(false);
      return Length(0);
  }
}

Point LengthUnit::convertFromUnit(const QPointF& point) const {
  switch (mUnit) {
    case LengthUnit_t::Millimeters:
      return Point::fromMm(point);
    case LengthUnit_t::Micrometers:
      return Point::fromMm(point / (qreal)1000);
    case LengthUnit_t::Nanometers:
      return Point::fromMm(point / (qreal)1000000);
    case LengthUnit_t::Inches:
      return Point::fromInch(point);
    case LengthUnit_t::Mils:
      return Point::fromMil(point);
    default:
      qCritical() << "Unhandled switch-case in LengthUnit::convertFromUnit():"
                  << static_cast<int>(mUnit);
      Q_ASSERT(false);
      return Point(Length(0), Length(0));
  }
}

// Static Methods

LengthUnit LengthUnit::fromString(const QString& str) {
  foreach (const LengthUnit& unit, getAllUnits()) {
    if (unit.toStr() == str) {
      return unit;
    }
  }
  throw RuntimeError(
      __FILE__, __LINE__,
      QString(LengthUnit::tr("Invalid length unit: \"%1\"")).arg(str));
}

LengthUnit LengthUnit::fromIndex(int index) {
  if (index >= static_cast<int>(LengthUnit_t::_COUNT))
    throw LogicError(__FILE__, __LINE__, QString::number(index));

  return LengthUnit(static_cast<LengthUnit_t>(index));
}

QList<LengthUnit> LengthUnit::getAllUnits() noexcept {
  QList<LengthUnit> list;
  for (int i = 0; i < static_cast<int>(LengthUnit_t::_COUNT); i++)
    list.append(LengthUnit(static_cast<LengthUnit_t>(i)));
  return list;
}

std::optional<LengthUnit> LengthUnit::extractFromExpression(
    QString& expression) noexcept {
  foreach (const LengthUnit& unit, LengthUnit::getAllUnits()) {
    foreach (const QString& suffix, unit.getUserInputSuffixes()) {
      if (expression.endsWith(suffix)) {
        expression.chop(suffix.length());
        return unit;
      }
    }
  }
  return std::nullopt;
}

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
std::unique_ptr<SExpression> serialize(const LengthUnit& obj) {
  return SExpression::createToken(obj.toStr());
}

template <>
LengthUnit deserialize(const SExpression& node) {
  return LengthUnit::fromString(node.getValue());  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
