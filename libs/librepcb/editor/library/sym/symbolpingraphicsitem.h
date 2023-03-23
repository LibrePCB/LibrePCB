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

#ifndef LIBREPCB_EDITOR_SYMBOLPINGRAPHICSITEM_H
#define LIBREPCB_EDITOR_SYMBOLPINGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/library/sym/symbolpin.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;
class ComponentSymbolVariantItem;

namespace editor {

class IF_GraphicsLayerProvider;
class LineGraphicsItem;
class PrimitiveCircleGraphicsItem;
class PrimitiveTextGraphicsItem;

/*******************************************************************************
 *  Class SymbolPinGraphicsItem
 ******************************************************************************/

/**
 * @brief The SymbolPinGraphicsItem class
 */
class SymbolPinGraphicsItem final : public QGraphicsItemGroup {
public:
  // Constructors / Destructor
  SymbolPinGraphicsItem() = delete;
  SymbolPinGraphicsItem(const SymbolPinGraphicsItem& other) = delete;
  SymbolPinGraphicsItem(
      std::shared_ptr<SymbolPin> pin, const IF_GraphicsLayerProvider& lp,
      std::shared_ptr<const Component> cmp = nullptr,
      std::shared_ptr<const ComponentSymbolVariantItem> cmpItem = nullptr,
      QGraphicsItem* parent = nullptr) noexcept;
  ~SymbolPinGraphicsItem() noexcept;

  // Getters
  const std::shared_ptr<SymbolPin>& getPin() noexcept { return mPin; }

  // General Methods
  void updateText() noexcept;

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override;

  // Operator Overloadings
  SymbolPinGraphicsItem& operator=(const SymbolPinGraphicsItem& rhs) = delete;

private:  // Methods
  void pinEdited(const SymbolPin& pin, SymbolPin::Event event) noexcept;
  virtual QVariant itemChange(GraphicsItemChange change,
                              const QVariant& value) noexcept override;
  void setLength(const UnsignedLength& length) noexcept;
  void updateTextPosition() noexcept;

private:  // Data
  std::shared_ptr<SymbolPin> mPin;
  const IF_GraphicsLayerProvider& mLayerProvider;
  std::shared_ptr<const Component> mComponent;  // Can be nullptr.
  std::shared_ptr<const ComponentSymbolVariantItem> mItem;  // Can be nullptr.
  QScopedPointer<PrimitiveCircleGraphicsItem> mCircleGraphicsItem;
  QScopedPointer<LineGraphicsItem> mLineGraphicsItem;
  QScopedPointer<PrimitiveTextGraphicsItem> mTextGraphicsItem;

  // Slots
  SymbolPin::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
