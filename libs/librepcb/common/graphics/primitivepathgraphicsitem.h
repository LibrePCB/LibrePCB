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

#ifndef LIBREPCB_PRIMITIVEPATHGRAPHICSITEM_H
#define LIBREPCB_PRIMITIVEPATHGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../graphics/graphicslayer.h"
#include "../units/all_length_units.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class PrimitivePathGraphicsItem
 ******************************************************************************/

/**
 * @brief The PrimitivePathGraphicsItem class
 *
 * @author ubruhin
 * @date 2017-05-28
 */
class PrimitivePathGraphicsItem : public QGraphicsItem,
                                  public IF_GraphicsLayerObserver {
public:
  // Constructors / Destructor
  // PrimitivePathGraphicsItem() = delete;
  PrimitivePathGraphicsItem(const PrimitivePathGraphicsItem& other) = delete;
  explicit PrimitivePathGraphicsItem(QGraphicsItem* parent = nullptr) noexcept;
  virtual ~PrimitivePathGraphicsItem() noexcept;

  // Setters
  void setPosition(const Point& pos) noexcept;
  void setRotation(const Angle& rot) noexcept;
  void setPath(const QPainterPath& path) noexcept;
  void setLineWidth(const UnsignedLength& width) noexcept;
  void setLineLayer(const GraphicsLayer* layer) noexcept;
  void setFillLayer(const GraphicsLayer* layer) noexcept;

  // Inherited from IF_LayerObserver
  void layerColorChanged(const GraphicsLayer& layer,
                         const QColor&        newColor) noexcept override;
  void layerHighlightColorChanged(const GraphicsLayer& layer,
                                  const QColor& newColor) noexcept override;
  void layerVisibleChanged(const GraphicsLayer& layer,
                           bool                 newVisible) noexcept override;
  void layerEnabledChanged(const GraphicsLayer& layer,
                           bool                 newEnabled) noexcept override;
  void layerDestroyed(const GraphicsLayer& layer) noexcept override;

  // Inherited from QGraphicsItem
  QRectF       boundingRect() const noexcept override { return mBoundingRect; }
  QPainterPath shape() const noexcept override { return mShape; }
  void         paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                     QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  PrimitivePathGraphicsItem& operator=(const PrimitivePathGraphicsItem& rhs) =
      delete;

private:  // Methods
  void updateColors() noexcept;
  void updateBoundingRectAndShape() noexcept;
  void updateVisibility() noexcept;

private:  // Data
  const GraphicsLayer* mLineLayer;
  const GraphicsLayer* mFillLayer;
  QPen                 mPen;
  QPen                 mPenHighlighted;
  QBrush               mBrush;
  QBrush               mBrushHighlighted;
  QPainterPath         mPainterPath;
  QRectF               mBoundingRect;
  QPainterPath         mShape;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_PRIMITIVEPATHGRAPHICSITEM_H
