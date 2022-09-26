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

#ifndef LIBREPCB_CORE_SGI_SYMBOLPIN_H
#define LIBREPCB_CORE_SGI_SYMBOLPIN_H

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

class GraphicsLayer;
class LineGraphicsItem;
class PrimitiveCircleGraphicsItem;
class PrimitiveTextGraphicsItem;
class SI_SymbolPin;

/*******************************************************************************
 *  Class SGI_SymbolPin
 ******************************************************************************/

/**
 * @brief The SGI_SymbolPin class
 */
class SGI_SymbolPin final : public SGI_Base {
public:
  // Constructors / Destructor
  SGI_SymbolPin() = delete;
  SGI_SymbolPin(const SGI_SymbolPin& other) = delete;
  explicit SGI_SymbolPin(SI_SymbolPin& pin) noexcept;
  ~SGI_SymbolPin() noexcept;

  // General Methods
  void updateTransform() noexcept;
  void updateData() noexcept;
  void updateSelection() noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override;
  QPainterPath shape() const noexcept override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  SGI_SymbolPin& operator=(const SGI_SymbolPin& rhs) = delete;

private:  // Methods
  GraphicsLayer* getLayer(const QString& name) const noexcept;

private:  // Data
  SI_SymbolPin& mPin;
  QScopedPointer<PrimitiveCircleGraphicsItem> mCircleGraphicsItem;
  QScopedPointer<LineGraphicsItem> mLineGraphicsItem;
  QScopedPointer<PrimitiveTextGraphicsItem> mTextGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
