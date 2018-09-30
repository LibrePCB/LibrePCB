/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
  // check format of string (only accept EXACT matches!)
  QRegularExpression re(
      "^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$");
  QRegularExpressionMatch match =
      re.match(str, 0, QRegularExpression::PartialPreferCompleteMatch);
  if (!match.hasMatch()) return false;

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
