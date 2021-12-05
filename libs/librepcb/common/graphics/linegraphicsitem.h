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

#ifndef LIBREPCB_COMMON_LINEGRAPHICSITEM_H
#define LIBREPCB_COMMON_LINEGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../graphics/graphicslayer.h"
#include "../units/length.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class Point;

/*******************************************************************************
 *  Class LineGraphicsItem
 ******************************************************************************/

/**
 * @brief The LineGraphicsItem class
 */
class LineGraphicsItem final : public QGraphicsItem {
public:
  // Constructors / Destructor
  // LineGraphicsItem() = delete;
  LineGraphicsItem(const LineGraphicsItem& other) = delete;
  explicit LineGraphicsItem(QGraphicsItem* parent = nullptr) noexcept;
  ~LineGraphicsItem() noexcept;

  // Setters
  void setPosition(const Point& pos) noexcept;
  void setRotation(const Angle& rot) noexcept;
  void setLine(const Point& p1, const Point& p2) noexcept;
  void setLineWidth(const UnsignedLength& width) noexcept;
  void setLayer(const GraphicsLayer* layer) noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override { return mBoundingRect; }
  QPainterPath shape() const noexcept override { return mShape; }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  LineGraphicsItem& operator=(const LineGraphicsItem& rhs) = delete;

private:  // Methods
  void layerEdited(const GraphicsLayer& layer,
                   GraphicsLayer::Event event) noexcept;
  void updateBoundingRectAndShape() noexcept;

private:  // Data
  const GraphicsLayer* mLayer;
  QPen mPen;
  QPen mPenHighlighted;
  QLineF mLine;
  QRectF mBoundingRect;
  QPainterPath mShape;

  // Slots
  GraphicsLayer::OnEditedSlot mOnLayerEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
