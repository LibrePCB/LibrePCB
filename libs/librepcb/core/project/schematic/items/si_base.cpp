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
#include "si_base.h"

#include "../../../graphics/graphicsscene.h"
#include "../../project.h"
#include "../graphicsitems/sgi_base.h"
#include "../schematic.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SI_Base::SI_Base(Schematic& schematic) noexcept
  : QObject(&schematic),
    mSchematic(schematic),
    mIsAddedToSchematic(false),
    mIsSelected(false) {
}

SI_Base::~SI_Base() noexcept {
  Q_ASSERT(!mIsAddedToSchematic);
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

Project& SI_Base::getProject() const noexcept {
  return mSchematic.getProject();
}

Circuit& SI_Base::getCircuit() const noexcept {
  return mSchematic.getProject().getCircuit();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SI_Base::setSelected(bool selected) noexcept {
  mIsSelected = selected;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SI_Base::addToSchematic(QGraphicsItem* item) noexcept {
  Q_ASSERT(!mIsAddedToSchematic);
  if (item) {
    mSchematic.getGraphicsScene().addItem(*item);
  }
  mIsAddedToSchematic = true;
}

void SI_Base::removeFromSchematic(QGraphicsItem* item) noexcept {
  Q_ASSERT(mIsAddedToSchematic);
  if (item) {
    mSchematic.getGraphicsScene().removeItem(*item);
  }
  mIsAddedToSchematic = false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
