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

#ifndef LIBREPCB_EDITOR_STROKETEXTGRAPHICSITEM_H
#define LIBREPCB_EDITOR_STROKETEXTGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/geometry/stroketext.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class IF_GraphicsLayerProvider;
class OriginCrossGraphicsItem;
class PrimitivePathGraphicsItem;

/*******************************************************************************
 *  Class StrokeTextGraphicsItem
 ******************************************************************************/

/**
 * @brief The StrokeTextGraphicsItem class is the graphical representation of a
 *        librepcb::StrokeText
 */
class StrokeTextGraphicsItem final : public QGraphicsItemGroup {
public:
  // Constructors / Destructor
  StrokeTextGraphicsItem() = delete;
  StrokeTextGraphicsItem(const StrokeTextGraphicsItem& other) = delete;
  StrokeTextGraphicsItem(StrokeText& text, const IF_GraphicsLayerProvider& lp,
                         const StrokeFont& font,
                         QGraphicsItem* parent = nullptr) noexcept;
  virtual ~StrokeTextGraphicsItem() noexcept;

  // Getters
  StrokeText& getText() noexcept { return mText; }

  // General Methods
  void setTextOverride(const tl::optional<QString>& text) noexcept;

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override;

  // Operator Overloadings
  StrokeTextGraphicsItem& operator=(const StrokeTextGraphicsItem& rhs) = delete;

private:  // Methods
  void strokeTextEdited(const StrokeText& text,
                        StrokeText::Event event) noexcept;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant& value) noexcept override;
  void updateLayer(const Layer& layer) noexcept;
  void updateText() noexcept;
  void updateTransform() noexcept;

private:  // Data
  StrokeText& mText;
  const IF_GraphicsLayerProvider& mLayerProvider;
  const StrokeFont& mFont;
  tl::optional<QString> mTextOverride;
  QScopedPointer<PrimitivePathGraphicsItem> mPathGraphicsItem;
  QScopedPointer<OriginCrossGraphicsItem> mOriginCrossGraphicsItem;

  // Slots
  StrokeText::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
