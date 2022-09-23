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

#ifndef LIBREPCB_CORE_ATTRTYPECURRENT_H
#define LIBREPCB_CORE_ATTRTYPECURRENT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "attributetype.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class AttrTypeCurrent
 ******************************************************************************/

/**
 * @brief The AttrTypeCurrent class
 */
class AttrTypeCurrent final : public AttributeType {
public:
  // Constructors / Destructor
  AttrTypeCurrent(const AttrTypeCurrent& other) = delete;

  bool isValueValid(const QString& value) const noexcept;
  QString valueFromTr(const QString& value) const noexcept;
  QString printableValueTr(const QString& value,
                           const AttributeUnit* unit = nullptr) const noexcept;
  static const AttrTypeCurrent& instance() noexcept {
    static AttrTypeCurrent x;
    return x;
  }

  // Operator Overloadings
  AttrTypeCurrent& operator=(const AttrTypeCurrent& rhs) = delete;

private:
  // Constructors / Destructor
  AttrTypeCurrent() noexcept;
  ~AttrTypeCurrent() noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
