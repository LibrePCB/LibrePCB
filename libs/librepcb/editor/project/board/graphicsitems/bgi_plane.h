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

#ifndef LIBREPCB_EDITOR_BGI_PLANE_H
#define LIBREPCB_EDITOR_BGI_PLANE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../graphics/graphicslayer.h"

#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class NetSignal;
class Path;
class Polygon;

namespace editor {

class GraphicsLayerList;

/*******************************************************************************
 *  Class BGI_Plane
 ******************************************************************************/

/**
 * @brief The BGI_Plane class
 */
class BGI_Plane final : public QGraphicsItem {
public:
  // Constructors / Destructor
  BGI_Plane() = delete;
  BGI_Plane(const BGI_Plane& other) = delete;
  BGI_Plane(BI_Plane& plane, const GraphicsLayerList& layers,
            std::shared_ptr<const QSet<const NetSignal*>>
                highlightedNetSignals) noexcept;
  virtual ~BGI_Plane() noexcept;

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

  // General Methods
  BI_Plane& getPlane() noexcept { return mPlane; }

  // Inherited from QGraphicsItem
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
  BGI_Plane& operator=(const BGI_Plane& rhs) = delete;

private:  // Methods
  void planeEdited(const BI_Plane& obj, BI_Plane::Event event) noexcept;
  void layerEdited(const GraphicsLayer& layer,
                   GraphicsLayer::Event event) noexcept;
  void updateOutlineAndFragments() noexcept;
  void updateLayer() noexcept;
  void updateVisibility() noexcept;
  void updateBoundingRectMargin() noexcept;

private:  // Data
  // General Attributes
  BI_Plane& mPlane;
  const GraphicsLayerList& mLayers;
  std::shared_ptr<const QSet<const NetSignal*>> mHighlightedNetSignals;

  // Cached Attributes
  std::shared_ptr<const GraphicsLayer> mLayer;
  QRectF mBoundingRect;
  qreal mBoundingRectMarginPx;
  QPainterPath mShape;
  QPainterPath mOutline;
  QVector<QPainterPath> mAreas;
  qreal mLineWidthPx;
  qreal mVertexHandleRadiusPx;
  struct VertexHandle {
    Point pos;
    qreal maxGlowRadiusPx;
  };
  QVector<VertexHandle> mVertexHandles;

  // Slots
  BI_Plane::OnEditedSlot mOnEditedSlot;
  GraphicsLayer::OnEditedSlot mOnLayerEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
