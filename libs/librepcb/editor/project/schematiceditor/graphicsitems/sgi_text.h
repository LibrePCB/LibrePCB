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

#ifndef LIBREPCB_EDITOR_SGI_TEXT_H
#define LIBREPCB_EDITOR_SGI_TEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "sgi_symbol.h"

#include <librepcb/core/project/schematic/items/si_text.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class IF_GraphicsLayerProvider;
class LineGraphicsItem;
class TextGraphicsItem;

/*******************************************************************************
 *  Class SGI_Text
 ******************************************************************************/

/**
 * @brief The SGI_Text class
 */
class SGI_Text final : public QGraphicsItemGroup {
public:
  // Constructors / Destructor
  SGI_Text() = delete;
  SGI_Text(const SGI_Text& other) = delete;
  SGI_Text(SI_Text& text, std::weak_ptr<SGI_Symbol> symbolItem,
           const IF_GraphicsLayerProvider& lp) noexcept;
  virtual ~SGI_Text() noexcept;

  // General Methods
  SI_Text& getText() noexcept { return mText; }
  const std::weak_ptr<SGI_Symbol>& getSymbolGraphicsItem() noexcept {
    return mSymbolGraphicsItem;
  }

  // Inherited from QGraphicsItem
  virtual QPainterPath shape() const noexcept override;

  // Operator Overloadings
  SGI_Text& operator=(const SGI_Text& rhs) = delete;

private:  // Methods
  void textEdited(const SI_Text& obj, SI_Text::Event event) noexcept;
  void symbolGraphicsItemEdited(const SGI_Symbol& obj,
                                SGI_Symbol::Event event) noexcept;
  virtual QVariant itemChange(GraphicsItemChange change,
                              const QVariant& value) noexcept override;
  void updateText() noexcept;
  void updateAnchorLayer() noexcept;
  void updateAnchorLine() noexcept;

private:  // Data
  SI_Text& mText;
  std::weak_ptr<SGI_Symbol> mSymbolGraphicsItem;
  const IF_GraphicsLayerProvider& mLayerProvider;
  QScopedPointer<TextGraphicsItem> mTextGraphicsItem;
  QScopedPointer<LineGraphicsItem> mAnchorGraphicsItem;

  // Slots
  SI_Text::OnEditedSlot mOnEditedSlot;
  SGI_Symbol::OnEditedSlot mOnSymbolEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
