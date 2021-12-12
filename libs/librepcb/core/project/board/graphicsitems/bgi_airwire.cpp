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
#include "bgi_airwire.h"

#include "../../circuit/netsignal.h"
#include "../board.h"
#include "../boardlayerstack.h"
#include "../items/bi_airwire.h"

#include <QPrinter>
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BGI_AirWire::BGI_AirWire(BI_AirWire& airwire) noexcept
  : BGI_Base(), mAirWire(airwire), mLayer(nullptr) {
  mLayer = getLayer(GraphicsLayer::sBoardAirWires);
  setZValue(Board::ZValue_AirWires);
  updateCacheAndRepaint();
}

BGI_AirWire::~BGI_AirWire() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool BGI_AirWire::isSelectable() const noexcept {
  return mLayer && mLayer->isVisible();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BGI_AirWire::updateCacheAndRepaint() noexcept {
  prepareGeometryChange();

  mLines.clear();
  if (mAirWire.isVertical()) {
    Length size(200000);
    Point p1 = mAirWire.getP1() + Point(size, size);
    Point p2 = mAirWire.getP1() - Point(size, size);
    Point p3 = mAirWire.getP1() + Point(size, -size);
    Point p4 = mAirWire.getP1() - Point(size, -size);
    mLines.append(QLineF(p1.toPxQPointF(), p2.toPxQPointF()));
    mLines.append(QLineF(p3.toPxQPointF(), p4.toPxQPointF()));
    mBoundingRect = QRectF(p1.toPxQPointF(), p2.toPxQPointF()).normalized();
  } else {
    mLines.append(
        QLineF(mAirWire.getP1().toPxQPointF(), mAirWire.getP2().toPxQPointF()));
    mBoundingRect =
        QRectF(mAirWire.getP1().toPxQPointF(), mAirWire.getP2().toPxQPointF())
            .normalized();
  }

  update();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void BGI_AirWire::paint(QPainter* painter,
                        const QStyleOptionGraphicsItem* option,
                        QWidget* widget) {
  Q_UNUSED(widget);

  bool highlight =
      mAirWire.isSelected() || mAirWire.getNetSignal().isHighlighted();
  const qreal lod =
      option->levelOfDetailFromTransform(painter->worldTransform());

  // draw line
  if (mLayer && mLayer->isVisible()) {
    qreal width = highlight ? 3 / lod : 0;  // highlighted airwires are thicker
    QPen pen(mLayer->getColor(highlight), width, Qt::SolidLine, Qt::RoundCap);
    painter->setPen(pen);
    painter->drawLines(mLines);
    if (mLines.count() > 1) {
      painter->setBrush(Qt::NoBrush);
      painter->drawEllipse(mBoundingRect);
    }
  }

#ifdef QT_DEBUG
  GraphicsLayer* layer =
      getLayer(GraphicsLayer::sDebugGraphicsItemsBoundingRects);
  Q_ASSERT(layer);
  if (layer && layer->isVisible()) {
    // draw bounding rect
    painter->setPen(QPen(layer->getColor(highlight), 0));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(mBoundingRect);
  }
#endif
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

GraphicsLayer* BGI_AirWire::getLayer(const QString& name) const noexcept {
  return mAirWire.getBoard().getLayerStack().getLayer(name);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
