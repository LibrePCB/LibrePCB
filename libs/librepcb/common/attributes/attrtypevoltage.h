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

#ifndef LIBREPCB_COMMON_ATTRTYPEVOLTAGE_H
#define LIBREPCB_COMMON_ATTRTYPEVOLTAGE_H

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
 *  Class AttrTypeVoltage
 ******************************************************************************/

/**
 * @brief The AttrTypeVoltage class
 */
class AttrTypeVoltage final : public AttributeType {
public:
  // Constructors / Destructor
  AttrTypeVoltage(const AttrTypeVoltage& other) = delete;

  bool isValueValid(const QString& value) const noexcept;
  QString valueFromTr(const QString& value) const noexcept;
  QString printableValueTr(const QString& value,
                           const AttributeUnit* unit = nullptr) const noexcept;
  static const AttrTypeVoltage& instance() noexcept {
    static AttrTypeVoltage x;
    return x;
  }

  // Operator Overloadings
  AttrTypeVoltage& operator=(const AttrTypeVoltage& rhs) = delete;

private:
  // Constructors / Destructor
  AttrTypeVoltage() noexcept;
  ~AttrTypeVoltage() noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
