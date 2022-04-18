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

#ifndef LIBREPCB_CORE_SGI_SYMBOL_H
#define LIBREPCB_CORE_SGI_SYMBOL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "sgi_base.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class CircleGraphicsItem;
class GraphicsLayer;
class OriginCrossGraphicsItem;
class PolygonGraphicsItem;
class PrimitiveTextGraphicsItem;
class SI_Symbol;

/*******************************************************************************
 *  Class SGI_Symbol
 ******************************************************************************/

/**
 * @brief The SGI_Symbol class
 */
class SGI_Symbol final : public SGI_Base {
public:
  // Constructors / Destructor
  SGI_Symbol() = delete;
  SGI_Symbol(const SGI_Symbol& other) = delete;
  explicit SGI_Symbol(SI_Symbol& symbol) noexcept;
  ~SGI_Symbol() noexcept;

  // General Methods
  void setPosition(const Point& pos) noexcept;
  void setSelected(bool selected) noexcept;
  void updateRotationAndMirror() noexcept;
  void updateAllTexts() noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override { return mBoundingRect; }
  QPainterPath shape() const noexcept override { return mShape; }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  SGI_Symbol& operator=(const SGI_Symbol& rhs) = delete;

private:  // Data
  SI_Symbol& mSymbol;
  std::shared_ptr<OriginCrossGraphicsItem> mOriginCrossGraphicsItem;
  QVector<std::shared_ptr<CircleGraphicsItem>> mCircleGraphicsItems;
  QVector<std::shared_ptr<PolygonGraphicsItem>> mPolygonGraphicsItems;
  QVector<std::shared_ptr<PrimitiveTextGraphicsItem>> mTextGraphicsItems;
  QRectF mBoundingRect;
  QPainterPath mShape;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
