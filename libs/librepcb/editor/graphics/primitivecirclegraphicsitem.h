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

#ifndef LIBREPCB_EDITOR_PRIMITIVECIRCLEGRAPHICSITEM_H
#define LIBREPCB_EDITOR_PRIMITIVECIRCLEGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/types/length.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Point;

namespace editor {

/*******************************************************************************
 *  Class PrimitiveCircleGraphicsItem
 ******************************************************************************/

/**
 * @brief The PrimitiveCircleGraphicsItem class
 */
class PrimitiveCircleGraphicsItem : public QGraphicsItem {
public:
  // Types
  enum class ShapeMode {
    /// Both the line stroke (with its specified width) and the filled area
    /// are used as shape, if the corresponding layers are set and visible.
    StrokeAndAreaByLayer,

    /// Only the area within the painter path is used as shape.
    FilledOutline,
  };

  // Constructors / Destructor
  // PrimitiveCircleGraphicsItem() = delete;
  PrimitiveCircleGraphicsItem(const PrimitiveCircleGraphicsItem& other) =
      delete;
  explicit PrimitiveCircleGraphicsItem(
      QGraphicsItem* parent = nullptr) noexcept;
  virtual ~PrimitiveCircleGraphicsItem() noexcept;

  // Setters
  void setPosition(const Point& pos) noexcept;
  void setDiameter(const UnsignedLength& dia) noexcept;
  void setLineWidth(const UnsignedLength& width) noexcept;
  void setLineLayer(const GraphicsLayer* layer) noexcept;
  void setFillLayer(const GraphicsLayer* layer) noexcept;
  void setShapeMode(ShapeMode mode) noexcept;

  // Inherited from QGraphicsItem
  virtual QRectF boundingRect() const noexcept override {
    return mBoundingRect;
  }
  virtual QPainterPath shape() const noexcept override;
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                     QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  PrimitiveCircleGraphicsItem& operator=(
      const PrimitiveCircleGraphicsItem& rhs) = delete;

private:  // Methods
  void layerEdited(const GraphicsLayer& layer,
                   GraphicsLayer::Event event) noexcept;
  void updateColors() noexcept;
  void updateBoundingRectAndShape() noexcept;
  void updateVisibility() noexcept;

private:  // Data
  const GraphicsLayer* mLineLayer;
  const GraphicsLayer* mFillLayer;
  ShapeMode mShapeMode;
  QPen mPen;
  QPen mPenHighlighted;
  QBrush mBrush;
  QBrush mBrushHighlighted;
  QRectF mCircleRect;
  QRectF mBoundingRect;
  QPainterPath mShape;

  // Slots
  GraphicsLayer::OnEditedSlot mOnLayerEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
