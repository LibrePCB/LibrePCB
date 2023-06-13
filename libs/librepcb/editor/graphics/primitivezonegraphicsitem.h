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

#ifndef LIBREPCB_EDITOR_PRIMITIVEZONEGRAPHICSITEM_H
#define LIBREPCB_EDITOR_PRIMITIVEZONEGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "primitivepathgraphicsitem.h"

#include <librepcb/core/geometry/path.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

namespace editor {

class IF_GraphicsLayerProvider;

/*******************************************************************************
 *  Class PrimitiveZoneGraphicsItem
 ******************************************************************************/

/**
 * @brief The PrimitiveZoneGraphicsItem class
 */
class PrimitiveZoneGraphicsItem : public QGraphicsItem {
public:
  // Constructors / Destructor
  PrimitiveZoneGraphicsItem() = delete;
  PrimitiveZoneGraphicsItem(const PrimitiveZoneGraphicsItem& other) = delete;
  PrimitiveZoneGraphicsItem(const IF_GraphicsLayerProvider& lp,
                            QGraphicsItem* parent = nullptr) noexcept;
  virtual ~PrimitiveZoneGraphicsItem() noexcept;

  // Getters

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
  void setPosition(const Point& pos) noexcept;
  void setRotation(const Angle& rot) noexcept;
  void setAllLayers(const QSet<const Layer*>& layers) noexcept;
  void setEnabledLayers(const QSet<const Layer*>& layers) noexcept;
  void setOutline(const Path& path) noexcept;

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
  QRectF boundingRect() const noexcept override {
    return mBoundingRect +
        QMarginsF(mBoundingRectMarginPx, mBoundingRectMarginPx,
                  mBoundingRectMarginPx, mBoundingRectMarginPx);
  }
  QPainterPath shape() const noexcept override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  PrimitiveZoneGraphicsItem& operator=(const PrimitiveZoneGraphicsItem& rhs) =
      delete;

private:  // Methods
  void layerEdited(const GraphicsLayer& layer,
                   GraphicsLayer::Event event) noexcept;
  void updateColors() noexcept;
  void updateBoundingRectAndShape() noexcept;
  void updateBoundingRectMargin() noexcept;
  void updateVisibility() noexcept;

private:  // Data
  const IF_GraphicsLayerProvider& mLayerProvider;
  QVector<std::shared_ptr<GraphicsLayer>> mAllGraphicsLayers;
  QVector<std::shared_ptr<GraphicsLayer>> mEnabledGraphicsLayers;
  Path mOutline;
  bool mEditable;

  // Cached attributes
  std::shared_ptr<GraphicsLayer> mLayer;
  QPen mPen;
  QPen mPenHighlighted;
  QBrush mBrush;
  QBrush mBrushHighlighted;
  QPainterPath mPainterPath;
  QRectF mBoundingRect;
  qreal mBoundingRectMarginPx;
  QPainterPath mShape;
  qreal mVertexHandleRadiusPx;
  struct VertexHandle {
    Point pos;
    qreal maxGlowRadiusPx;
  };
  QVector<VertexHandle> mVertexHandles;

  // Slots
  GraphicsLayer::OnEditedSlot mOnLayerEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
