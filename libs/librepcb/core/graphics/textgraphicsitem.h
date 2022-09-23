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

#ifndef LIBREPCB_CORE_TEXTGRAPHICSITEM_H
#define LIBREPCB_CORE_TEXTGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../geometry/text.h"
#include "primitivetextgraphicsitem.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class AttributeProvider;
class IF_GraphicsLayerProvider;
class OriginCrossGraphicsItem;

/*******************************************************************************
 *  Class TextGraphicsItem
 ******************************************************************************/

/**
 * @brief The TextGraphicsItem class is the graphical representation of a
 *        librepcb::Text
 */
class TextGraphicsItem final : public PrimitiveTextGraphicsItem {
public:
  // Constructors / Destructor
  TextGraphicsItem() = delete;
  TextGraphicsItem(const TextGraphicsItem& other) = delete;
  TextGraphicsItem(Text& text, const IF_GraphicsLayerProvider& lp,
                   QGraphicsItem* parent = nullptr) noexcept;
  ~TextGraphicsItem() noexcept;

  // Getters
  Text& getText() noexcept { return mText; }

  // Setters
  void setRotation(const Angle& rot) noexcept override;
  void setAlignment(const Alignment& align) noexcept override;

  // General Methods
  void setAttributeProvider(const AttributeProvider* provider) noexcept;
  void updateText() noexcept;

  // Operator Overloadings
  TextGraphicsItem& operator=(const TextGraphicsItem& rhs) = delete;

private:  // Methods
  void textEdited(const Text& text, Text::Event event) noexcept;
  void setRotationAndAlignment(Angle rotation, Alignment align) noexcept;

private:  // Data
  Text& mText;
  const IF_GraphicsLayerProvider& mLayerProvider;
  QScopedPointer<OriginCrossGraphicsItem> mOriginCrossGraphicsItem;

  /// Object for substituting placeholders in text
  const AttributeProvider* mAttributeProvider;

  // Slots
  Text::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
