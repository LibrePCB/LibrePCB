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
#include "version.h"

#include "../exceptions.h"
#include "../qtcompat.h"
#include "../serialization/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool Version::isPrefixOf(const Version& other) const noexcept {
  if ((other.mNumbers.count() >= mNumbers.count()) && (mNumbers.count() > 0)) {
    for (int i = 0; i < mNumbers.count(); i++) {
      if (mNumbers.at(i) != other.mNumbers.at(i)) {
        return false;
      }
    }
    return true;
  } else {
    return false;
  }
}

QString Version::toStr() const noexcept {
  return toPrettyStr(0);
}

QString Version::toPrettyStr(int minSegCount, int maxSegCount) const noexcept {
  Q_ASSERT(maxSegCount >= minSegCount);

  QString str;
  for (int i = 0; i < qMin(qMax(mNumbers.count(), minSegCount), maxSegCount);
       i++) {
    if (i > 0) str.append(".");
    str.append(QString::number((i < mNumbers.count()) ? mNumbers.at(i) : 0));
  }
  return str;
}

QString Version::toComparableStr() const noexcept {
  QString str;
  if (mNumbers.count() > 0) {
    for (int i = 0; i < 10; i++) {
      int number = (mNumbers.count() > i) ? mNumbers.at(i) : 0;
      if (i > 0) str.append(".");
      str.append(QString("%1").arg(number, 5, 10, QLatin1Char('0')));
    }
    Q_ASSERT(str.length() == 59);
  }
  return str;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

bool Version::isValid(const QString& str) noexcept {
  tl::optional<Version> version = tryFromString(str);
  return version.has_value();
}

Version Version::fromString(const QString& str) {
  tl::optional<Version> version = tryFromString(str);
  if (version) {
    return *version;
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(Version::tr("Invalid version number: \"%1\"")).arg(str));
  }
}

tl::optional<Version> Version::tryFromString(const QString& str) noexcept {
  QVector<uint> numbers;
  // split and convert to integer
  QStringList splitted =
      str.split('.', QtCompat::keepEmptyParts(), Qt::CaseSensitive);
  foreach (const QString& numberStr, splitted) {
    bool ok = false;
    uint number = numberStr.toUInt(&ok);
    if ((!ok) || (number > 99999)) {
      return tl::nullopt;
    }
    numbers.append(number);
  }
  // remove trailing zeros
  for (int i = numbers.count() - 1; i > 0; i--) {
    if (numbers.at(i) != 0) break;
    numbers.removeAt(i);
  }
  // check number count
  if (numbers.empty() || (numbers.count() > 10)) {
    return tl::nullopt;
  }
  return Version(numbers);
}

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
std::unique_ptr<SExpression> serialize(const Version& obj) {
  return SExpression::createString(obj.toStr());
}

template <>
Version deserialize(const SExpression& node) {
  return Version::fromString(node.getValue());  // can throw
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
