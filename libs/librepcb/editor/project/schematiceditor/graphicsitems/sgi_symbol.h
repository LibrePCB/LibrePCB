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

#ifndef LIBREPCB_EDITOR_SGI_SYMBOL_H
#define LIBREPCB_EDITOR_SGI_SYMBOL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/project/schematic/items/si_symbol.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Point;

namespace editor {

class CircleGraphicsItem;
class IF_GraphicsLayerProvider;
class OriginCrossGraphicsItem;
class PolygonGraphicsItem;

/*******************************************************************************
 *  Class SGI_Symbol
 ******************************************************************************/

/**
 * @brief The SGI_Symbol class
 */
class SGI_Symbol final : public QGraphicsItemGroup {
public:
  // Signals
  enum class Event {
    PositionChanged,
    SelectionChanged,
  };
  Signal<SGI_Symbol, Event> onEdited;
  typedef Slot<SGI_Symbol, Event> OnEditedSlot;

  // Constructors / Destructor
  SGI_Symbol() = delete;
  SGI_Symbol(const SGI_Symbol& other) = delete;
  SGI_Symbol(SI_Symbol& symbol, const IF_GraphicsLayerProvider& lp) noexcept;
  virtual ~SGI_Symbol() noexcept;

  // General Methods
  SI_Symbol& getSymbol() noexcept { return mSymbol; }

  // Inherited from QGraphicsItem
  QPainterPath shape() const noexcept override { return mShape; }

  // Operator Overloadings
  SGI_Symbol& operator=(const SGI_Symbol& rhs) = delete;

private:  // Methods
  void symbolEdited(const SI_Symbol& obj, SI_Symbol::Event event) noexcept;
  void updatePosition() noexcept;
  void updateRotationAndMirrored() noexcept;
  virtual QVariant itemChange(GraphicsItemChange change,
                              const QVariant& value) noexcept override;

private:  // Data
  SI_Symbol& mSymbol;
  std::shared_ptr<OriginCrossGraphicsItem> mOriginCrossGraphicsItem;
  QVector<std::shared_ptr<CircleGraphicsItem>> mCircleGraphicsItems;
  QVector<std::shared_ptr<PolygonGraphicsItem>> mPolygonGraphicsItems;
  QPainterPath mShape;

  // Slots
  SI_Symbol::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
