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

#include "../../../graphics/graphicslayer.h"
#include "../../../graphics/graphicslayerlist.h"
#include "../boardgraphicsscene.h"

#include <librepcb/core/project/board/items/bi_airwire.h>
#include <librepcb/core/project/board/items/bi_netline.h>
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

BGI_AirWire::BGI_AirWire(BI_AirWire& airwire, const GraphicsLayerList& layers,
                         std::shared_ptr<const QSet<const NetSignal*>>
                             highlightedNetSignals) noexcept
  : QGraphicsItem(),
    mAirWire(airwire),
    mHighlightedNetSignals(highlightedNetSignals),
    mLayer(layers.get(Theme::Color::sBoardAirWires)),
    mOnLayerEditedSlot(*this, &BGI_AirWire::layerEdited) {
  setZValue(BoardGraphicsScene::ZValue_AirWires);

  if (mAirWire.isVertical()) {
    Length size(200000);
    Point p1 = mAirWire.getP1().getPosition() + Point(size, size);
    Point p2 = mAirWire.getP1().getPosition() - Point(size, size);
    Point p3 = mAirWire.getP1().getPosition() + Point(size, -size);
    Point p4 = mAirWire.getP1().getPosition() - Point(size, -size);
    mLines.append(QLineF(p1.toPxQPointF(), p2.toPxQPointF()));
    mLines.append(QLineF(p3.toPxQPointF(), p4.toPxQPointF()));
    mBoundingRect = QRectF(p1.toPxQPointF(), p2.toPxQPointF()).normalized();
  } else {
    mLines.append(QLineF(mAirWire.getP1().getPosition().toPxQPointF(),
                         mAirWire.getP2().getPosition().toPxQPointF()));
    mBoundingRect = QRectF(mAirWire.getP1().getPosition().toPxQPointF(),
                           mAirWire.getP2().getPosition().toPxQPointF())
                        .normalized();
  }

  setVisible(mLayer && mLayer->isVisible());

  if (mLayer) {
    mLayer->onEdited.attach(mOnLayerEditedSlot);
  }
}

BGI_AirWire::~BGI_AirWire() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void BGI_AirWire::paint(QPainter* painter,
                        const QStyleOptionGraphicsItem* option,
                        QWidget* widget) {
  Q_UNUSED(widget);

  const bool highlight = option->state.testFlag(QStyle::State_Selected) ||
      mHighlightedNetSignals->contains(&mAirWire.getNetSignal());
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
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BGI_AirWire::layerEdited(const GraphicsLayer& layer,
                              GraphicsLayer::Event event) noexcept {
  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
      update();
      break;
    case GraphicsLayer::Event::HighlightColorChanged:
      update();
      break;
    case GraphicsLayer::Event::VisibleChanged:
    case GraphicsLayer::Event::EnabledChanged:
      setVisible(layer.isVisible());
      break;
    default:
      break;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
