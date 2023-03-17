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

#ifndef LIBREPCB_CORE_SI_POLYGON_H
#define LIBREPCB_CORE_SI_POLYGON_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../types/point.h"
#include "../../../types/uuid.h"
#include "si_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Polygon;
class Schematic;

/*******************************************************************************
 *  Class SI_Polygon
 ******************************************************************************/

/**
 * @brief The SI_Polygon class represents a polygon in a schematic
 */
class SI_Polygon final : public SI_Base {
  Q_OBJECT

public:
  // Constructors / Destructor
  SI_Polygon() = delete;
  SI_Polygon(const SI_Polygon& other) = delete;
  SI_Polygon(Schematic& schematic, const Polygon& polygon);
  ~SI_Polygon() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept;
  Polygon& getPolygon() noexcept { return *mPolygon; }
  const Polygon& getPolygon() const noexcept { return *mPolygon; }

  // General Methods
  void addToSchematic() override;
  void removeFromSchematic() override;

  // Operator Overloadings
  SI_Polygon& operator=(const SI_Polygon& rhs) = delete;

private:  // Attributes
  QScopedPointer<Polygon> mPolygon;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
