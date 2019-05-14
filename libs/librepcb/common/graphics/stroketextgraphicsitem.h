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

#ifndef LIBREPCB_STROKETEXTGRAPHICSITEM_H
#define LIBREPCB_STROKETEXTGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../geometry/stroketext.h"
#include "primitivepathgraphicsitem.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class OriginCrossGraphicsItem;
class IF_GraphicsLayerProvider;

/*******************************************************************************
 *  Class StrokeTextGraphicsItem
 ******************************************************************************/

/**
 * @brief The StrokeTextGraphicsItem class is the graphical representation of a
 *        librepcb::StrokeText
 */
class StrokeTextGraphicsItem final : public PrimitivePathGraphicsItem {
public:
  // Constructors / Destructor
  StrokeTextGraphicsItem()                                    = delete;
  StrokeTextGraphicsItem(const StrokeTextGraphicsItem& other) = delete;
  StrokeTextGraphicsItem(StrokeText& text, const IF_GraphicsLayerProvider& lp,
                         QGraphicsItem* parent = nullptr) noexcept;
  ~StrokeTextGraphicsItem() noexcept;

  // Getters
  StrokeText& getText() noexcept { return mText; }

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override;

  // Operator Overloadings
  StrokeTextGraphicsItem& operator=(const StrokeTextGraphicsItem& rhs) = delete;

private:  // Methods
  void     strokeTextEdited(const StrokeText& text,
                            StrokeText::Event event) noexcept;
  void     updateLayer(const GraphicsLayerName& layerName) noexcept;
  void     updateTransform() noexcept;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant&    value) noexcept override;

private:  // Data
  StrokeText&                             mText;
  const IF_GraphicsLayerProvider&         mLayerProvider;
  QScopedPointer<OriginCrossGraphicsItem> mOriginCrossGraphicsItem;

  // Slots
  StrokeText::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_STROKETEXTGRAPHICSITEM_H
