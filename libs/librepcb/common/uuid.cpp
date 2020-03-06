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
#include "uuid.h"

#include <QRegularExpressionValidator>
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

bool Uuid::isValid(const QString& str) noexcept {
  // Note: This used to be done using a RegEx, but when profiling and
  // optimizing the library rescan code we found that a manually unrolled
  // comparison loop performs much better than the previous RegEx.
  // See https://github.com/LibrePCB/LibrePCB/pull/651 for more details.
  if (str.length() != 36) return false;

  // Helper function
  auto isLowerHex = [](const QChar& chr) {
    return (chr >= QChar('0') && chr <= QChar('9')) ||
           (chr >= QChar('a') && chr <= QChar('f'));
  };

  if (!isLowerHex(str[0])) return false;
  if (!isLowerHex(str[1])) return false;
  if (!isLowerHex(str[2])) return false;
  if (!isLowerHex(str[3])) return false;
  if (!isLowerHex(str[4])) return false;
  if (!isLowerHex(str[5])) return false;
  if (!isLowerHex(str[6])) return false;
  if (!isLowerHex(str[7])) return false;
  if (str[8] != QChar('-')) return false;
  if (!isLowerHex(str[9])) return false;
  if (!isLowerHex(str[10])) return false;
  if (!isLowerHex(str[11])) return false;
  if (!isLowerHex(str[12])) return false;
  if (str[13] != QChar('-')) return false;
  if (!isLowerHex(str[14])) return false;
  if (!isLowerHex(str[15])) return false;
  if (!isLowerHex(str[16])) return false;
  if (!isLowerHex(str[17])) return false;
  if (str[18] != QChar('-')) return false;
  if (!isLowerHex(str[19])) return false;
  if (!isLowerHex(str[20])) return false;
  if (!isLowerHex(str[21])) return false;
  if (!isLowerHex(str[22])) return false;
  if (str[23] != QChar('-')) return false;
  if (!isLowerHex(str[24])) return false;
  if (!isLowerHex(str[25])) return false;
  if (!isLowerHex(str[26])) return false;
  if (!isLowerHex(str[27])) return false;
  if (!isLowerHex(str[28])) return false;
  if (!isLowerHex(str[29])) return false;
  if (!isLowerHex(str[30])) return false;
  if (!isLowerHex(str[31])) return false;
  if (!isLowerHex(str[32])) return false;
  if (!isLowerHex(str[33])) return false;
  if (!isLowerHex(str[34])) return false;
  if (!isLowerHex(str[35])) return false;

  // check type of uuid
  QUuid quuid(str);
  if (quuid.isNull()) return false;
  if (quuid.variant() != QUuid::DCE) return false;
  if (quuid.version() != QUuid::Random) return false;

  return true;
}

Uuid Uuid::createRandom() noexcept {
  QString str =
      QUuid::createUuid().toString().remove("{").remove("}").toLower();
  if (isValid(str)) {
    return Uuid(str);
  } else {
    qFatal("Not able to generate valid random UUID!");  // calls abort()!
  }
}

Uuid Uuid::fromString(const QString& str) {
  if (isValid(str)) {
    return Uuid(str);
  } else {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("String is not a valid UUID: \"%1\"")).arg(str));
  }
}

tl::optional<Uuid> Uuid::tryFromString(const QString& str) noexcept {
  if (isValid(str)) {
    return Uuid(str);
  } else {
    return tl::nullopt;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
