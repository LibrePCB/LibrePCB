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

#ifndef LIBREPCB_EDITOR_BGI_POLYGON_H
#define LIBREPCB_EDITOR_BGI_POLYGON_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/project/board/items/bi_polygon.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsLayerList;
class PolygonGraphicsItem;

/*******************************************************************************
 *  Class BGI_Polygon
 ******************************************************************************/

/**
 * @brief The BGI_Polygon class
 */
class BGI_Polygon final : public QGraphicsItemGroup {
public:
  // Constructors / Destructor
  BGI_Polygon() = delete;
  BGI_Polygon(const BGI_Polygon& other) = delete;
  BGI_Polygon(BI_Polygon& polygon, const GraphicsLayerList& layers) noexcept;
  virtual ~BGI_Polygon() noexcept;

  // General Methods
  BI_Polygon& getPolygon() noexcept { return mPolygon; }
  const PolygonGraphicsItem& getGraphicsItem() const noexcept {
    return *mGraphicsItem;
  }

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override;

  // Operator Overloadings
  BGI_Polygon& operator=(const BGI_Polygon& rhs) = delete;

private:  // Methods
  void polygonEdited(const BI_Polygon& obj, BI_Polygon::Event event) noexcept;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant& value) noexcept override;
  void updateZValue() noexcept;
  void updateEditable() noexcept;

private:  // Data
  BI_Polygon& mPolygon;
  Polygon mPolygonObj;
  QScopedPointer<PolygonGraphicsItem> mGraphicsItem;

  // Slots
  BI_Polygon::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
