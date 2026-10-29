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

class GraphicsLayerList;

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
  PolygonGraphicsItem(Polygon& polygon, const GraphicsLayerList& layers,
                      QGraphicsItem* parent = nullptr) noexcept;
  ~PolygonGraphicsItem() noexcept override;

  // Getters
  Polygon& getObj() noexcept { return mPolygon; }
  bool isInEditMode() const noexcept { return mInEditMode; }

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

  // General Methods

  /**
   * Enable edit mode
   *
   * When the item is in edit mode, vertex handles will be shown to indicate
   * that they can be moved. If not in edit mode, handles will not be shown.
   *
   * @note  Edit mode is automatically left when the item gets deselected.
   *        And when calling this method while the item is not selected,
   *        it has no effect.
   */
  void enterEditMode() noexcept;

  /**
   * @brief Disable edit mode
   *
   * Only needs to be called while the item is selected, as edit mode is
   * left automatically when the item is deselected.
   */
  void leaveEditMode() noexcept;

  // Inherited Methods
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant& value) noexcept override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = nullptr) noexcept override;

  // Operator Overloadings
  PolygonGraphicsItem& operator=(const PolygonGraphicsItem& rhs) = delete;

private:  // Methods
  void polygonEdited(const Polygon& polygon, Polygon::Event event) noexcept;
  void updateFillLayer() noexcept;
  void updatePath() noexcept;
  void updateZValue() noexcept;
  void updateBoundingRectMargin() noexcept;

private:  // Data
  Polygon& mPolygon;
  const GraphicsLayerList& mLayers;
  bool mInEditMode;

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
