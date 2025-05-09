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
#include "primitiveholegraphicsitem.h"

#include "graphicslayer.h"
#include "graphicslayerlist.h"
#include "origincrossgraphicsitem.h"
#include "primitivepathgraphicsitem.h"

#include <librepcb/core/workspace/theme.h>

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

PrimitiveHoleGraphicsItem::PrimitiveHoleGraphicsItem(
    const GraphicsLayerList& layers, bool originCrossesVisible,
    QGraphicsItem* parent) noexcept
  : QGraphicsItemGroup(parent),
    mHoleLayer(layers.get(Theme::Color::sBoardHoles)),
    mHoleGraphicsItem(new PrimitivePathGraphicsItem(this)),
    mStopMaskGraphicsItemBot(new PrimitivePathGraphicsItem(this)),
    mStopMaskGraphicsItemTop(new PrimitivePathGraphicsItem(this)),
    mOriginCrossGraphicsItemStart(new OriginCrossGraphicsItem(this)),
    mOriginCrossGraphicsItemEnd(new OriginCrossGraphicsItem(this)) {
  setFlag(QGraphicsItem::ItemHasNoContents, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setZValue(5);

  // Setup hole item.
  mHoleGraphicsItem->setLineLayer(mHoleLayer);
  mHoleGraphicsItem->setLineWidth(UnsignedLength(0));
  mHoleGraphicsItem->setShapeMode(
      PrimitivePathGraphicsItem::ShapeMode::FilledOutline);
  mHoleGraphicsItem->setZValue(0);

  // Setup stop mask items.
  mStopMaskGraphicsItemBot->setFillLayer(
      layers.get(Theme::Color::sBoardStopMaskBot));
  mStopMaskGraphicsItemBot->setZValue(-10);
  mStopMaskGraphicsItemTop->setFillLayer(
      layers.get(Theme::Color::sBoardStopMaskTop));
  mStopMaskGraphicsItemTop->setZValue(10);

  // Setup origin crosses.
  if (originCrossesVisible) {
    for (auto item :
         {&mOriginCrossGraphicsItemStart, &mOriginCrossGraphicsItemEnd}) {
      (*item)->setLayer(mHoleLayer);
      (*item)->setRotation(Angle::deg45());
      (*item)->setZValue(20);
    }
  }
}

PrimitiveHoleGraphicsItem::~PrimitiveHoleGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PrimitiveHoleGraphicsItem::setHole(
    const NonEmptyPath& path, const PositiveLength& diameter,
    const std::optional<Length>& stopMaskOffset) noexcept {
  // Update hole outline.
  mHoleGraphicsItem->setPath(
      Path::toQPainterPathPx(path->toOutlineStrokes(diameter), false));

  // Update stop mask area.
  Length stopMaskDiameter;
  if (stopMaskOffset) {
    stopMaskDiameter = (*diameter) + (*stopMaskOffset) + (*stopMaskOffset);
  }
  const QPainterPath stopMask = (stopMaskDiameter > 0)
      ? Path::toQPainterPathPx(
            path->toOutlineStrokes(PositiveLength(stopMaskDiameter)), false)
      : QPainterPath();
  mStopMaskGraphicsItemBot->setPath(stopMask);
  mStopMaskGraphicsItemTop->setPath(stopMask);

  // Update origin crosses position.
  mOriginCrossGraphicsItemStart->setPosition(
      path->getVertices().first().getPos());
  mOriginCrossGraphicsItemEnd->setPosition(path->getVertices().last().getPos());

  // Update origin crosses size.
  const UnsignedLength originCrossSize =
      positiveToUnsigned(diameter) + UnsignedLength(500000);
  mOriginCrossGraphicsItemStart->setSize(originCrossSize);
  mOriginCrossGraphicsItemEnd->setSize(originCrossSize);

  // Update visibility of second origin cross.
  if (path->getVertices().count() > 1) {
    mOriginCrossGraphicsItemEnd->setLayer(mHoleLayer);
  } else {
    mOriginCrossGraphicsItemEnd->setLayer(nullptr);
  }
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath PrimitiveHoleGraphicsItem::shape() const noexcept {
  return mHoleGraphicsItem->shape();
}

QVariant PrimitiveHoleGraphicsItem::itemChange(GraphicsItemChange change,
                                               const QVariant& value) noexcept {
  if ((change == ItemSelectedHasChanged) && mHoleGraphicsItem &&
      mStopMaskGraphicsItemBot && mStopMaskGraphicsItemTop &&
      mOriginCrossGraphicsItemStart && mOriginCrossGraphicsItemEnd) {
    mHoleGraphicsItem->setSelected(value.toBool());
    mStopMaskGraphicsItemBot->setSelected(value.toBool());
    mStopMaskGraphicsItemTop->setSelected(value.toBool());
    mOriginCrossGraphicsItemStart->setSelected(value.toBool());
    mOriginCrossGraphicsItemEnd->setSelected(value.toBool());
  }
  return QGraphicsItem::itemChange(change, value);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
