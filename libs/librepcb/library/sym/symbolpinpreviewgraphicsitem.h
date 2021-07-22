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

#ifndef LIBREPCB_LIBRARY_SYMBOLPINPREVIEWGRAPHICSITEM_H
#define LIBREPCB_LIBRARY_SYMBOLPINPREVIEWGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../cmp/cmpsigpindisplaytype.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayer;
class IF_GraphicsLayerProvider;

namespace library {

class SymbolPin;
class ComponentSignal;

/*******************************************************************************
 *  Class SymbolPinPreviewGraphicsItem
 ******************************************************************************/

/**
 * @brief The SymbolPinPreviewGraphicsItem class
 */
class SymbolPinPreviewGraphicsItem final : public QGraphicsItem {
public:
  // Constructors / Destructor
  SymbolPinPreviewGraphicsItem() = delete;
  SymbolPinPreviewGraphicsItem(const SymbolPinPreviewGraphicsItem& other) =
      delete;
  explicit SymbolPinPreviewGraphicsItem(
      const IF_GraphicsLayerProvider& layerProvider, const SymbolPin& pin,
      const ComponentSignal* compSignal,
      const CmpSigPinDisplayType& displayType) noexcept;
  ~SymbolPinPreviewGraphicsItem() noexcept;

  // Setters
  void setDrawBoundingRect(bool enable) noexcept { mDrawBoundingRect = enable; }

  // General Methods
  void updateCacheAndRepaint() noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override { return mBoundingRect; }
  QPainterPath shape() const noexcept override { return mShape; }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  SymbolPinPreviewGraphicsItem& operator=(
      const SymbolPinPreviewGraphicsItem& rhs) = delete;

private:
  // General Attributes
  const SymbolPin& mPin;
  const ComponentSignal* mComponentSignal;
  CmpSigPinDisplayType mDisplayType;
  GraphicsLayer* mCircleLayer;
  GraphicsLayer* mLineLayer;
  GraphicsLayer* mTextLayer;
  QFont mFont;
  qreal mRadiusPx;
  bool mDrawBoundingRect;

  // Cached Attributes
  QStaticText mStaticText;
  bool mRotate180;
  QRectF mBoundingRect;
  QPointF mTextOrigin;
  QRectF mTextBoundingRect;
  QPainterPath mShape;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_SYMBOLPINPREVIEWGRAPHICSITEM_H
