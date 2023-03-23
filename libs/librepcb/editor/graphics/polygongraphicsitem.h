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

#ifndef LIBREPCB_EDITOR_POLYGONGRAPHICSITEM_H
#define LIBREPCB_EDITOR_POLYGONGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "primitivepathgraphicsitem.h"

#include <librepcb/core/geometry/polygon.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

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
  PolygonGraphicsItem() = delete;
  PolygonGraphicsItem(const PolygonGraphicsItem& other) = delete;
  PolygonGraphicsItem(Polygon& polygon, const IF_GraphicsLayerProvider& lp,
                      QGraphicsItem* parent = nullptr) noexcept;
  virtual ~PolygonGraphicsItem() noexcept;

  // Getters
  Polygon& getPolygon() noexcept { return mPolygon; }

  /// Get the line segment at a specific position
  ///
  /// @param pos    The position to check for lines.
  /// @return       The index of the vertex *after* the line under the cursor.
  ///               So for the first line segment, index 1 is returned. If no
  ///               line is located under the specified position, -1 is
  ///               returned.
  int getLineIndexAtPosition(const Point& pos) const noexcept;

  /// Get the vertices at a specific position
  ///
  /// @param pos    The position to check for vertices.
  /// @return       All indices of the vertices at the specified position.
  QVector<int> getVertexIndicesAtPosition(const Point& pos) const noexcept;

  // Setters

  /**
   * Enable/disable editing mode when selected
   *
   * If the item is editable and selected, vertex handles will be shown to
   * indicate that they can be moved. If not editable, handles will not be
   * shown.
   *
   * @param editable  Whether the polygon is (visually) editable or not.
   */
  void setEditable(bool editable) noexcept;

  // Inherited Methods
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant& value) noexcept override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  PolygonGraphicsItem& operator=(const PolygonGraphicsItem& rhs) = delete;

private:  // Methods
  void polygonEdited(const Polygon& polygon, Polygon::Event event) noexcept;
  void updateFillLayer() noexcept;
  void updatePath() noexcept;
  void updateBoundingRectMargin() noexcept;

private:  // Data
  Polygon& mPolygon;
  const IF_GraphicsLayerProvider& mLayerProvider;
  bool mEditable;

  // Cached attributes
  qreal mVertexHandleRadiusPx;
  struct VertexHandle {
    Point pos;
    qreal maxGlowRadiusPx;
  };
  QVector<VertexHandle> mVertexHandles;

  // Slots
  Polygon::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
