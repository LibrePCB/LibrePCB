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

#ifndef LIBREPCB_COMMON_PRIMITIVETEXTGRAPHICSITEM_H
#define LIBREPCB_COMMON_PRIMITIVETEXTGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../alignment.h"
#include "../graphics/graphicslayer.h"
#include "../units/all_length_units.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class PrimitiveTextGraphicsItem
 ******************************************************************************/

/**
 * @brief The PrimitiveTextGraphicsItem class is the graphical representation of
 * a text
 */
class PrimitiveTextGraphicsItem : public QGraphicsItem {
public:
  // Types
  enum class Font { SansSerif, Monospace };

  // Constructors / Destructor
  // PrimitiveTextGraphicsItem() = delete;
  PrimitiveTextGraphicsItem(const PrimitiveTextGraphicsItem& other) = delete;
  explicit PrimitiveTextGraphicsItem(QGraphicsItem* parent = nullptr) noexcept;
  virtual ~PrimitiveTextGraphicsItem() noexcept;

  // Setters
  void setPosition(const Point& pos) noexcept;
  void setRotation(const Angle& rot) noexcept;
  void setText(const QString& text) noexcept;
  void setHeight(const PositiveLength& height) noexcept;
  void setAlignment(const Alignment& align) noexcept;
  void setFont(Font font) noexcept;
  void setLayer(const GraphicsLayer* layer) noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override { return mBoundingRect; }
  QPainterPath shape() const noexcept override { return mShape; }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  PrimitiveTextGraphicsItem& operator=(const PrimitiveTextGraphicsItem& rhs) =
      delete;

private:  // Methods
  void layerEdited(const GraphicsLayer& layer,
                   GraphicsLayer::Event event) noexcept;
  void updateBoundingRectAndShape() noexcept;

private:  // Data
  const GraphicsLayer* mLayer;
  QString mText;
  Alignment mAlignment;
  QFont mFont;
  QPen mPen;
  QPen mPenHighlighted;
  int mTextFlags;
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
