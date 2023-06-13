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
#include "zonegraphicsitem.h"

#include <librepcb/core/types/layer.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ZoneGraphicsItem::ZoneGraphicsItem(Zone& zone,
                                   const IF_GraphicsLayerProvider& lp,
                                   QGraphicsItem* parent) noexcept
  : PrimitiveZoneGraphicsItem(lp, parent),
    mZone(zone),
    mOnEditedSlot(*this, &ZoneGraphicsItem::zoneEdited) {
  setOutline(mZone.getOutline());

  // Register to the zone to get updates.
  mZone.onEdited.attach(mOnEditedSlot);
}

ZoneGraphicsItem::~ZoneGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ZoneGraphicsItem::zoneEdited(const Zone& zone,
                                  Zone::Event event) noexcept {
  switch (event) {
    case Zone::Event::LayersChanged:
    case Zone::Event::RulesChanged:
      break;
    case Zone::Event::OutlineChanged:
      setOutline(zone.getOutline());
      break;
    default:
      qWarning() << "Unhandled switch-case in ZoneGraphicsItem::zoneEdited():"
                 << static_cast<int>(event);
      break;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
