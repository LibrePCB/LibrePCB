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

#ifndef LIBREPCB_GRIDPROPERTIES_H
#define LIBREPCB_GRIDPROPERTIES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "fileio/serializableobject.h"
#include "units/all_length_units.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class GridProperties
 ******************************************************************************/

/**
 * @brief The GridProperties class
 */
class GridProperties final : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(GridProperties)

public:
  // Types
  enum class Type_t { Off, Lines, Dots };

  // Constructors / Destructor
  GridProperties() noexcept;
  GridProperties(const SExpression& node, const Version& fileFormat);
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

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operators
  GridProperties& operator=(const GridProperties& rhs) noexcept;

private:  // Data
  Type_t mType;
  PositiveLength mInterval;
  LengthUnit mUnit;
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
inline SExpression serialize(const GridProperties::Type_t& obj) {
  switch (obj) {
    case GridProperties::Type_t::Off:
      return SExpression::createToken("off");
    case GridProperties::Type_t::Lines:
      return SExpression::createToken("lines");
    case GridProperties::Type_t::Dots:
      return SExpression::createToken("dots");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
inline GridProperties::Type_t deserialize(const SExpression& sexpr,
                                          const Version& fileFormat) {
  Q_UNUSED(fileFormat);
  QString str = sexpr.getValue();
  if (str == "off")
    return GridProperties::Type_t::Off;
  else if (str == "lines")
    return GridProperties::Type_t::Lines;
  else if (str == "dots")
    return GridProperties::Type_t::Dots;
  else
    throw RuntimeError(
        __FILE__, __LINE__,
        GridProperties::tr("Unknown grid type: \"%1\"").arg(str));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_GRIDPROPERTIES_H
