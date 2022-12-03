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

#ifndef LIBREPCB_CORE_GRIDPROPERTIES_H
#define LIBREPCB_CORE_GRIDPROPERTIES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "length.h"
#include "lengthunit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SExpression;

/*******************************************************************************
 *  Class GridProperties
 ******************************************************************************/

/**
 * @brief The GridProperties class
 */
class GridProperties final {
  Q_DECLARE_TR_FUNCTIONS(GridProperties)

public:
  // Types
  enum class Type_t { Off, Lines, Dots };

  // Constructors / Destructor
  GridProperties() noexcept;
  explicit GridProperties(const SExpression& node);
  GridProperties(Type_t type, const PositiveLength& interval,
                 const LengthUnit& unit) noexcept;
  GridProperties(const GridProperties& other) noexcept;
  ~GridProperties() noexcept;

  // Getters
  Type_t getType() const noexcept { return mType; }
  const PositiveLength& getInterval() const noexcept { return mInterval; }
  const LengthUnit& getUnit() const noexcept { return mUnit; }

  // Setters
  void setType(Type_t type) noexcept { mType = type; }
  void setInterval(const PositiveLength& interval) noexcept {
    mInterval = interval;
  }
  void setUnit(const LengthUnit& unit) noexcept { mUnit = unit; }

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operators
  GridProperties& operator=(const GridProperties& rhs) noexcept;

private:  // Data
  Type_t mType;
  PositiveLength mInterval;
  LengthUnit mUnit;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
