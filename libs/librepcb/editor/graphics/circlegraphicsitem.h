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

#ifndef LIBREPCB_EDITOR_CIRCLEGRAPHICSITEM_H
#define LIBREPCB_EDITOR_CIRCLEGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "primitivecirclegraphicsitem.h"

#include <librepcb/core/geometry/circle.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class IF_GraphicsLayerProvider;

namespace editor {

/*******************************************************************************
 *  Class CircleGraphicsItem
 ******************************************************************************/

/**
 * @brief The CircleGraphicsItem class
 */
class CircleGraphicsItem final : public PrimitiveCircleGraphicsItem {
public:
  // Constructors / Destructor
  CircleGraphicsItem() = delete;
  CircleGraphicsItem(const CircleGraphicsItem& other) = delete;
  CircleGraphicsItem(Circle& circle, const IF_GraphicsLayerProvider& lp,
                     QGraphicsItem* parent = nullptr) noexcept;
  virtual ~CircleGraphicsItem() noexcept;

  // Getters
  Circle& getCircle() noexcept { return mCircle; }

  // Operator Overloadings
  CircleGraphicsItem& operator=(const CircleGraphicsItem& rhs) = delete;

private:  // Methods
  void circleEdited(const Circle& circle, Circle::Event event) noexcept;
  void updateFillLayer() noexcept;

private:  // Data
  Circle& mCircle;
  const IF_GraphicsLayerProvider& mLayerProvider;

  // Slots
  Circle::OnEditedSlot mEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
