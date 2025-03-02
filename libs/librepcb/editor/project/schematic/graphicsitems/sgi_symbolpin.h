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

#ifndef LIBREPCB_EDITOR_SGI_SYMBOLPIN_H
#define LIBREPCB_EDITOR_SGI_SYMBOLPIN_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "sgi_symbol.h"

#include <librepcb/core/project/schematic/items/si_symbolpin.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsLayerList;
class LineGraphicsItem;
class PrimitiveCircleGraphicsItem;
class PrimitiveTextGraphicsItem;

/*******************************************************************************
 *  Class SGI_SymbolPin
 ******************************************************************************/

/**
 * @brief The SGI_SymbolPin class
 */
class SGI_SymbolPin final : public QGraphicsItemGroup {
  Q_DECLARE_TR_FUNCTIONS(SGI_SymbolPin)

public:
  // Constructors / Destructor
  SGI_SymbolPin() = delete;
  SGI_SymbolPin(const SGI_SymbolPin& other) = delete;
  SGI_SymbolPin(SI_SymbolPin& pin, std::weak_ptr<SGI_Symbol> symbolItem,
                const GraphicsLayerList& layers,
                std::shared_ptr<const QSet<const NetSignal*>>
                    highlightedNetSignals) noexcept;
  virtual ~SGI_SymbolPin() noexcept;

  // General Methods
  SI_SymbolPin& getPin() noexcept { return mPin; }
  const std::weak_ptr<SGI_Symbol>& getSymbolGraphicsItem() noexcept {
    return mSymbolGraphicsItem;
  }
  void updateHighlightedState() noexcept;

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override;

  // Operator Overloadings
  SGI_SymbolPin& operator=(const SGI_SymbolPin& rhs) = delete;

private:  // Methods
  void pinEdited(const SI_SymbolPin& obj, SI_SymbolPin::Event event) noexcept;
  void symbolGraphicsItemEdited(const SGI_Symbol& obj,
                                SGI_Symbol::Event event) noexcept;
  virtual QVariant itemChange(GraphicsItemChange change,
                              const QVariant& value) noexcept override;
  void updatePosition() noexcept;
  void updateRotation() noexcept;
  void updateJunction() noexcept;
  void updateName() noexcept;
  void updateNumbers() noexcept;
  void updateNumbersPosition() noexcept;
  void updateNumbersAlignment() noexcept;
  void updateToolTip() noexcept;

private:  // Data
  SI_SymbolPin& mPin;
  std::weak_ptr<SGI_Symbol> mSymbolGraphicsItem;
  const GraphicsLayerList& mLayers;
  std::shared_ptr<const QSet<const NetSignal*>> mHighlightedNetSignals;
  QScopedPointer<PrimitiveCircleGraphicsItem> mCircleGraphicsItem;
  QScopedPointer<LineGraphicsItem> mLineGraphicsItem;
  QScopedPointer<PrimitiveTextGraphicsItem> mNameGraphicsItem;
  QScopedPointer<PrimitiveTextGraphicsItem> mNumbersGraphicsItem;

  // Slots
  SI_SymbolPin::OnEditedSlot mOnPinEditedSlot;
  SGI_Symbol::OnEditedSlot mOnSymbolEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
