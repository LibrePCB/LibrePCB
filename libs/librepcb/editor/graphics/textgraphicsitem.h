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

#ifndef LIBREPCB_EDITOR_TEXTGRAPHICSITEM_H
#define LIBREPCB_EDITOR_TEXTGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/geometry/text.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsLayerList;
class OriginCrossGraphicsItem;
class PrimitiveTextGraphicsItem;

/*******************************************************************************
 *  Class TextGraphicsItem
 ******************************************************************************/

/**
 * @brief The TextGraphicsItem class is the graphical representation of a
 *        librepcb::Text
 */
class TextGraphicsItem final : public QGraphicsItemGroup {
public:
  // Constructors / Destructor
  TextGraphicsItem() = delete;
  TextGraphicsItem(const TextGraphicsItem& other) = delete;
  TextGraphicsItem(Text& text, const GraphicsLayerList& layers,
                   QGraphicsItem* parent = nullptr) noexcept;
  virtual ~TextGraphicsItem() noexcept;

  // Getters
  Text& getObj() noexcept { return mText; }

  // Setters
  void setTextOverride(const std::optional<QString>& text) noexcept;

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override;

  // Operator Overloadings
  TextGraphicsItem& operator=(const TextGraphicsItem& rhs) = delete;

private:  // Methods
  void textEdited(const Text& text, Text::Event event) noexcept;
  virtual QVariant itemChange(GraphicsItemChange change,
                              const QVariant& value) noexcept override;
  void updateText() noexcept;

private:  // Data
  Text& mText;
  const GraphicsLayerList& mLayers;
  std::optional<QString> mTextOverride;
  QScopedPointer<PrimitiveTextGraphicsItem> mTextGraphicsItem;
  QScopedPointer<OriginCrossGraphicsItem> mOriginCrossGraphicsItem;

  // Slots
  Text::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
