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
#include "holegraphicsitem.h"

#include "primitiveholegraphicsitem.h"

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

HoleGraphicsItem::HoleGraphicsItem(Hole& hole,
                                   const IF_GraphicsLayerProvider& lp,
                                   bool originCrossesVisible,
                                   QGraphicsItem* parent) noexcept
  : QGraphicsItemGroup(parent),
    mHole(hole),
    mGraphicsItem(
        new PrimitiveHoleGraphicsItem(lp, originCrossesVisible, this)),
    mOnEditedSlot(*this, &HoleGraphicsItem::holeEdited) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(5);

  updateHole();

  // register to the text to get attribute updates
  mHole.onEdited.attach(mOnEditedSlot);
}

HoleGraphicsItem::~HoleGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath HoleGraphicsItem::shape() const noexcept {
  return mGraphicsItem->shape();
}

QVariant HoleGraphicsItem::itemChange(GraphicsItemChange change,
                                      const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mGraphicsItem) {
    mGraphicsItem->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void HoleGraphicsItem::holeEdited(const Hole& hole,
                                  Hole::Event event) noexcept {
  Q_UNUSED(hole);

  switch (event) {
    case Hole::Event::PathChanged:
    case Hole::Event::DiameterChanged:
    case Hole::Event::StopMaskConfigChanged:
      updateHole();
      break;
    default:
      qWarning() << "Unhandled switch-case in HoleGraphicsItem::holeEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void HoleGraphicsItem::updateHole() noexcept {
  tl::optional<Length> stopMaskOffset;
  if (!mHole.getStopMaskConfig().isEnabled()) {
    stopMaskOffset = tl::nullopt;
  } else if (auto offset = mHole.getStopMaskConfig().getOffset()) {
    stopMaskOffset = *offset;
  } else {
    stopMaskOffset = Length(100000);  // Only for illustration.
  }
  mGraphicsItem->setHole(mHole.getPath(), mHole.getDiameter(), stopMaskOffset);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
