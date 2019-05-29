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

#ifndef LIBREPCB_POLYGONGRAPHICSITEM_H
#define LIBREPCB_POLYGONGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../geometry/polygon.h"
#include "primitivepathgraphicsitem.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class IF_GraphicsLayerProvider;

/*******************************************************************************
 *  Class PolygonGraphicsItem
 ******************************************************************************/

/**
 * @brief The PolygonGraphicsItem class
 */
class PolygonGraphicsItem final : public PrimitivePathGraphicsItem {
public:
  // Constructors / Destructor
  PolygonGraphicsItem()                                 = delete;
  PolygonGraphicsItem(const PolygonGraphicsItem& other) = delete;
  PolygonGraphicsItem(Polygon& polygon, const IF_GraphicsLayerProvider& lp,
                      QGraphicsItem* parent = nullptr) noexcept;
  ~PolygonGraphicsItem() noexcept;

  // Getters
  Polygon& getPolygon() noexcept { return mPolygon; }

  // Operator Overloadings
  PolygonGraphicsItem& operator=(const PolygonGraphicsItem& rhs) = delete;

private:  // Methods
  void polygonEdited(const Polygon& polygon, Polygon::Event event) noexcept;
  void updateFillLayer() noexcept;

private:  // Data
  Polygon&                        mPolygon;
  const IF_GraphicsLayerProvider& mLayerProvider;

  // Slots
  Polygon::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_POLYGONGRAPHICSITEM_H
