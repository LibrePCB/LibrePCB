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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bgi_hole.h"

#include "../../../graphics/primitiveholegraphicsitem.h"
#include "../boardgraphicsscene.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BGI_Hole::BGI_Hole(BI_Hole& hole, const GraphicsLayerList& layers) noexcept
  : QGraphicsItemGroup(),
    mHole(hole),
    mGraphicsItem(new PrimitiveHoleGraphicsItem(layers, true, this)),
    mOnEditedSlot(*this, &BGI_Hole::holeEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(BoardGraphicsScene::ZValue_Holes);

  updateHole();

  mHole.onEdited.attach(mOnEditedSlot);
}

BGI_Hole::~BGI_Hole() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath BGI_Hole::shape() const noexcept {
  return mGraphicsItem->shape();
}

QVariant BGI_Hole::itemChange(GraphicsItemChange change,
                              const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mGraphicsItem) {
    mGraphicsItem->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BGI_Hole::holeEdited(const BI_Hole& obj, BI_Hole::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case BI_Hole::Event::DiameterChanged:
    case BI_Hole::Event::PathChanged:
    case BI_Hole::Event::StopMaskOffsetChanged:
      updateHole();
      break;
    default:
      qWarning() << "Unhandled switch-case in BGI_Hole::holeEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void BGI_Hole::updateHole() noexcept {
  mGraphicsItem->setHole(mHole.getData().getPath(),
                         mHole.getData().getDiameter(),
                         mHole.getStopMaskOffset());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
