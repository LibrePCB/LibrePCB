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
#include "si_polygon.h"

#include "../../../geometry/polygon.h"
#include "../../project.h"
#include "../schematic.h"
#include "../schematiclayerprovider.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SI_Polygon::SI_Polygon(Schematic& schematic, const Polygon& polygon)
  : SI_Base(schematic), mPolygon(new Polygon(polygon)) {
}

SI_Polygon::~SI_Polygon() noexcept {
  mPolygon.reset();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const Uuid& SI_Polygon::getUuid() const noexcept {
  return mPolygon->getUuid();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_Polygon::addToSchematic() {
  if (isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  SI_Base::addToSchematic();
}

void SI_Polygon::removeFromSchematic() {
  if (!isAddedToSchematic()) {
    throw LogicError(__FILE__, __LINE__);
  }
  SI_Base::removeFromSchematic();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
