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

#ifndef LIBREPCB_EDITOR_ZONEGRAPHICSITEM_H
#define LIBREPCB_EDITOR_ZONEGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "primitivezonegraphicsitem.h"

#include <librepcb/core/geometry/zone.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsLayerList;

/*******************************************************************************
 *  Class ZoneGraphicsItem
 ******************************************************************************/

/**
 * @brief The ZoneGraphicsItem class
 */
class ZoneGraphicsItem final : public PrimitiveZoneGraphicsItem {
public:
  // Constructors / Destructor
  ZoneGraphicsItem() = delete;
  ZoneGraphicsItem(const ZoneGraphicsItem& other) = delete;
  ZoneGraphicsItem(Zone& zone, const GraphicsLayerList& layers,
                   QGraphicsItem* parent = nullptr) noexcept;
  virtual ~ZoneGraphicsItem() noexcept;

  // Getters
  Zone& getObj() noexcept { return mZone; }

  // Operator Overloadings
  ZoneGraphicsItem& operator=(const ZoneGraphicsItem& rhs) = delete;

private:  // Methods
  void zoneEdited(const Zone& zone, Zone::Event event) noexcept;

private:  // Data
  Zone& mZone;

  // Slots
  Zone::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
