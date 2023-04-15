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

#ifndef LIBREPCB_EDITOR_PRIMITIVEPATHGRAPHICSITEM_H
#define LIBREPCB_EDITOR_PRIMITIVEPATHGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "graphicslayer.h"

#include <librepcb/core/types/length.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class Point;

namespace editor {

/*******************************************************************************
 *  Class PrimitivePathGraphicsItem
 ******************************************************************************/

/**
 * @brief The PrimitivePathGraphicsItem class
 */
class PrimitivePathGraphicsItem : public QGraphicsItem {
public:
  using QGraphicsItem::setRotation;

  // Types
  enum class ShapeMode {
    /// No shape is set (empty area).
    None,

    /// Only the area within the painter path is used as shape.
    FilledOutline,

    /// Both the line stroke (with its specified width) and the filled area
    /// are used as shape, if the corresponding layers are set and visible.
    StrokeAndAreaByLayer,
  };

  // Constructors / Destructor
  // PrimitivePathGraphicsItem() = delete;
  PrimitivePathGraphicsItem(const PrimitivePathGraphicsItem& other) = delete;
  explicit PrimitivePathGraphicsItem(QGraphicsItem* parent = nullptr) noexcept;
  virtual ~PrimitivePathGraphicsItem() noexcept;

  // Setters
  void setPosition(const Point& pos) noexcept;
  void setRotation(const Angle& rot) noexcept;
  void setMirrored(bool mirrored) noexcept;
  void setPath(const QPainterPath& path) noexcept;
  void setLineWidth(const UnsignedLength& width) noexcept;
  void setLineLayer(const std::shared_ptr<GraphicsLayer>& layer) noexcept;
  void setFillLayer(const std::shared_ptr<GraphicsLayer>& layer) noexcept;
  void setShapeMode(ShapeMode mode) noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override {
    return mBoundingRect +
        QMarginsF(mBoundingRectMarginPx, mBoundingRectMarginPx,
                  mBoundingRectMarginPx, mBoundingRectMarginPx);
  }
  QPainterPath shape() const noexcept override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  PrimitivePathGraphicsItem& operator=(const PrimitivePathGraphicsItem& rhs) =
      delete;

private:  // Methods
  void layerEdited(const GraphicsLayer& layer,
                   GraphicsLayer::Event event) noexcept;
  void updateColors() noexcept;
  void updateBoundingRectAndShape() noexcept;
  void updateVisibility() noexcept;

protected:  // Data
  bool mMirror;
  std::shared_ptr<GraphicsLayer> mLineLayer;
  std::shared_ptr<GraphicsLayer> mFillLayer;
  ShapeMode mShapeMode;
  QPen mPen;
  QPen mPenHighlighted;
  QBrush mBrush;
  QBrush mBrushHighlighted;
  QPainterPath mPainterPath;
  QRectF mBoundingRect;
  qreal mBoundingRectMarginPx;
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
