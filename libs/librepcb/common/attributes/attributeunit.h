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

#ifndef LIBREPCB_ATTRIBUTEUNIT_H
#define LIBREPCB_ATTRIBUTEUNIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class AttributeUnit
 ******************************************************************************/

/**
 * @brief The AttributeUnit class
 */
class AttributeUnit final {
public:
  // Constructors / Destructor
  explicit AttributeUnit(const QString& name, const QString& symbolTr,
                         const QStringList& userInputSuffixes) noexcept;
  ~AttributeUnit() noexcept;

  // Getters
  const QString& getName() const noexcept { return mName; }
  const QString& getSymbolTr() const noexcept { return mSymbolTr; }
  const QStringList& getUserInputSuffixes() const noexcept {
    return mUserInputSuffixes;
  }

private:
  // make some methods inaccessible...
  AttributeUnit() = delete;
  AttributeUnit(const AttributeUnit& other) = delete;
  AttributeUnit& operator=(const AttributeUnit& rhs) = delete;

  // General Attributes
  QString mName;  ///< to convert from/to string, e.g. "millivolt"
  QString mSymbolTr;  ///< e.g. "mV"
  QStringList mUserInputSuffixes;  ///< user input suffixes, e.g. "k" or "meg"
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
inline SExpression serializeToSExpression(const AttributeUnit& obj) {
  return SExpression::createToken(obj.getName());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_ATTRIBUTEUNIT_H
