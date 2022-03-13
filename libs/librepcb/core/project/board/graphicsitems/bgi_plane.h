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

#ifndef LIBREPCB_CORE_BGI_PLANE_H
#define LIBREPCB_CORE_BGI_PLANE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../graphics/graphicslayer.h"
#include "bgi_base.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_Plane;
class Path;
class Polygon;

/*******************************************************************************
 *  Class BGI_Plane
 ******************************************************************************/

/**
 * @brief The BGI_Plane class
 */
class BGI_Plane final : public BGI_Base {
public:
  // Constructors / Destructor
  BGI_Plane() = delete;
  BGI_Plane(const BGI_Plane& other) = delete;
  explicit BGI_Plane(BI_Plane& plane) noexcept;
  ~BGI_Plane() noexcept;

  // Getters
  bool isSelectable() const noexcept;

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
  void updateCacheAndRepaint() noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept { return mBoundingRect; }
  QPainterPath shape() const noexcept { return mShape; }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0);

  // Operator Overloadings
  BGI_Plane& operator=(const BGI_Plane& rhs) = delete;

private:  // Methods
  GraphicsLayer* getLayer(const QString& name) const noexcept;
  void layerEdited(const GraphicsLayer& layer,
                   GraphicsLayer::Event event) noexcept;
  void updateVisibility() noexcept;

private:  // Data
  // General Attributes
  BI_Plane& mPlane;

  // Cached Attributes
  GraphicsLayer* mLayer;
  QRectF mBoundingRect;
  QPainterPath mShape;
  QPainterPath mOutline;
  QVector<QPainterPath> mAreas;
  qreal mLineWidthPx;
  qreal mVertexRadiusPx;

  // Slots
  GraphicsLayer::OnEditedSlot mOnLayerEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
