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

#include "../graphics/graphicslayer.h"
#include "../types/angle.h"
#include "origincrossgraphicsitem.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

HoleGraphicsItem::HoleGraphicsItem(const Hole& hole,
                                   const IF_GraphicsLayerProvider& lp,
                                   QGraphicsItem* parent) noexcept
  : PrimitivePathGraphicsItem(parent),
    mHole(hole),
    mLayerProvider(lp),
    mOriginCrossGraphicsItemStart(new OriginCrossGraphicsItem(this)),
    mOriginCrossGraphicsItemEnd(new OriginCrossGraphicsItem(this)),
    mOnEditedSlot(*this, &HoleGraphicsItem::holeEdited) {
  // setup origin crosses
  for (auto item :
       {&mOriginCrossGraphicsItemStart, &mOriginCrossGraphicsItemEnd}) {
    (*item)->setRotation(Angle::deg45());
    (*item)->setLayer(mLayerProvider.getLayer(GraphicsLayer::sTopReferences));
  }

  // update attributes
  setShape(mHole.getDiameter(), mHole.getPath());
  setLineLayer(mLayerProvider.getLayer(GraphicsLayer::sBoardDrillsNpth));
  setShapeMode(ShapeMode::FILLED_OUTLINE);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(5);

  // register to the text to get attribute updates
  mHole.onEdited.attach(mOnEditedSlot);
}

HoleGraphicsItem::~HoleGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void HoleGraphicsItem::holeEdited(const Hole& hole,
                                  Hole::Event event) noexcept {
  switch (event) {
    case Hole::Event::DiameterChanged:
    case Hole::Event::PathChanged:
      setShape(hole.getDiameter(), hole.getPath());
      break;
    default:
      qWarning() << "Unhandled switch-case in HoleGraphicsItem::holeEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void HoleGraphicsItem::setShape(const PositiveLength& diameter,
                                const NonEmptyPath& path) noexcept {
  setPath(Path::toQPainterPathPx(path->toOutlineStrokes(diameter), false));

  const UnsignedLength size =
      positiveToUnsigned(diameter) + UnsignedLength(500000);
  mOriginCrossGraphicsItemStart->setSize(size);
  mOriginCrossGraphicsItemEnd->setSize(size);

  mOriginCrossGraphicsItemStart->setPosition(
      path->getVertices().first().getPos());
  mOriginCrossGraphicsItemEnd->setPosition(path->getVertices().last().getPos());

  mOriginCrossGraphicsItemEnd->setVisible(path->getVertices().count() > 1);
}

QVariant HoleGraphicsItem::itemChange(GraphicsItemChange change,
                                      const QVariant& value) noexcept {
  if ((change == ItemSelectedChange) && mOriginCrossGraphicsItemStart &&
      mOriginCrossGraphicsItemEnd) {
    mOriginCrossGraphicsItemStart->setSelected(value.toBool());
    mOriginCrossGraphicsItemEnd->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
