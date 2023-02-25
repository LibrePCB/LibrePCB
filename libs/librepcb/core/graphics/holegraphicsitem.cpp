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

#include "../geometry/padgeometry.h"
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
                                   bool originCrossesVisible,
                                   QGraphicsItem* parent) noexcept
  : QGraphicsItem(parent),
    mHole(hole),
    mLayerProvider(lp),
    mAutoStopMaskOffset(0),
    mHoleGraphicsItem(new PrimitivePathGraphicsItem(this)),
    mStopMaskGraphicsItemBot(new PrimitivePathGraphicsItem(this)),
    mStopMaskGraphicsItemTop(new PrimitivePathGraphicsItem(this)),
    mOriginCrossGraphicsItemStart(new OriginCrossGraphicsItem(this)),
    mOriginCrossGraphicsItemEnd(new OriginCrossGraphicsItem(this)),
    mOnEditedSlot(*this, &HoleGraphicsItem::holeEdited) {
  // Setup hole item.
  mHoleGraphicsItem->setLineLayer(
      mLayerProvider.getLayer(GraphicsLayer::sBoardDrillsNpth));
  mHoleGraphicsItem->setShapeMode(
      PrimitivePathGraphicsItem::ShapeMode::FILLED_OUTLINE);
  mHoleGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
  mHoleGraphicsItem->setZValue(0);

  // Setup stop mask items.
  mStopMaskGraphicsItemBot->setFillLayer(
      mLayerProvider.getLayer(GraphicsLayer::sBotStopMask));
  mStopMaskGraphicsItemBot->setFlag(QGraphicsItem::ItemIsSelectable, true);
  mStopMaskGraphicsItemBot->setZValue(-10);
  mStopMaskGraphicsItemTop->setFillLayer(
      mLayerProvider.getLayer(GraphicsLayer::sTopStopMask));
  mStopMaskGraphicsItemTop->setFlag(QGraphicsItem::ItemIsSelectable, true);
  mStopMaskGraphicsItemTop->setZValue(10);

  // Setup origin crosses.
  if (originCrossesVisible) {
    for (auto item :
         {&mOriginCrossGraphicsItemStart, &mOriginCrossGraphicsItemEnd}) {
      (*item)->setLayer(
          mLayerProvider.getLayer(GraphicsLayer::sBoardDrillsNpth));
      (*item)->setRotation(Angle::deg45());
      (*item)->setZValue(20);
    }
  }

  // Setup this graphics item.
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setFlag(QGraphicsItem::ItemHasNoContents, false);
  setZValue(5);

  // Update attributes.
  updateGeometry();
  updateMasksVisibility();

  // register to the text to get attribute updates
  mHole.onEdited.attach(mOnEditedSlot);
}

HoleGraphicsItem::~HoleGraphicsItem() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void HoleGraphicsItem::setAutoStopMaskOffset(const Length& offset) noexcept {
  mAutoStopMaskOffset = offset;
  updateGeometry();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void HoleGraphicsItem::paint(QPainter* painter,
                             const QStyleOptionGraphicsItem* option,
                             QWidget* widget) noexcept {
  Q_UNUSED(painter);
  Q_UNUSED(option);
  Q_UNUSED(widget);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void HoleGraphicsItem::holeEdited(const Hole& hole,
                                  Hole::Event event) noexcept {
  Q_UNUSED(hole);

  switch (event) {
    case Hole::Event::DiameterChanged:
    case Hole::Event::PathChanged:
      updateGeometry();
      break;
    case Hole::Event::StopMaskConfigChanged:
      updateGeometry();
      updateMasksVisibility();
      break;
    default:
      qWarning() << "Unhandled switch-case in HoleGraphicsItem::holeEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void HoleGraphicsItem::updateGeometry() noexcept {
  prepareGeometryChange();
  mHoleGraphicsItem->setPath(Path::toQPainterPathPx(
      mHole.getPath()->toOutlineStrokes(mHole.getDiameter()), false));

  Length stopMaskOffset = mHole.getStopMaskConfig().getOffset()
      ? (*mHole.getStopMaskConfig().getOffset())
      : mAutoStopMaskOffset;
  const QPainterPath stopMask =
      PadGeometry::stroke(mHole.getDiameter(), mHole.getPath(), {})
          .withOffset(stopMaskOffset)
          .toFilledQPainterPathPx();
  mStopMaskGraphicsItemBot->setPath(stopMask);
  mStopMaskGraphicsItemTop->setPath(stopMask);

  const UnsignedLength size =
      positiveToUnsigned(mHole.getDiameter()) + UnsignedLength(500000);
  mOriginCrossGraphicsItemStart->setSize(size);
  mOriginCrossGraphicsItemEnd->setSize(
      (mHole.getPath()->getVertices().count() > 1) ? size : UnsignedLength(0));

  mOriginCrossGraphicsItemStart->setPosition(
      mHole.getPath()->getVertices().first().getPos());
  mOriginCrossGraphicsItemEnd->setPosition(
      mHole.getPath()->getVertices().last().getPos());
}

void HoleGraphicsItem::updateMasksVisibility() noexcept {
  mStopMaskGraphicsItemBot->setVisible(mHole.getStopMaskConfig().isEnabled());
  mStopMaskGraphicsItemTop->setVisible(mHole.getStopMaskConfig().isEnabled());
}

QVariant HoleGraphicsItem::itemChange(GraphicsItemChange change,
                                      const QVariant& value) noexcept {
  if ((change == ItemSelectedChange) && mHoleGraphicsItem &&
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
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
