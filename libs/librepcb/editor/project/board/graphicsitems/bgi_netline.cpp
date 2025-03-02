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
#include "bgi_netline.h"

#include "../../../graphics/graphicslayer.h"
#include "../boardgraphicsscene.h"

#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/utils/toolbox.h>

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

BGI_NetLine::BGI_NetLine(BI_NetLine& netline,
                         const IF_GraphicsLayerProvider& lp,
                         std::shared_ptr<const QSet<const NetSignal*>>
                             highlightedNetSignals) noexcept
  : QGraphicsItem(),
    mNetLine(netline),
    mLayerProvider(lp),
    mHighlightedNetSignals(highlightedNetSignals),
    mLayer(nullptr),
    mOnNetLineEditedSlot(*this, &BGI_NetLine::netLineEdited),
    mOnLayerEditedSlot(*this, &BGI_NetLine::layerEdited) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);

  updateLine();
  updateLayer();
  updateNetSignalName();
  updateVisibility();

  mNetLine.onEdited.attach(mOnNetLineEditedSlot);
}

BGI_NetLine::~BGI_NetLine() noexcept {
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

QPainterPath BGI_NetLine::shape() const noexcept {
  return (mLayer && mLayer->isVisible()) ? mShape : QPainterPath();
}

void BGI_NetLine::paint(QPainter* painter,
                        const QStyleOptionGraphicsItem* option,
                        QWidget* widget) noexcept {
  Q_UNUSED(widget);

  const NetSignal* netsignal = mNetLine.getNetSegment().getNetSignal();
  const bool highlight = option->state.testFlag(QStyle::State_Selected) ||
      mHighlightedNetSignals->contains(netsignal);

  // draw line
  if (mLayer->isVisible()) {
    QPen pen(mLayer->getColor(highlight), mNetLine.getWidth()->toPx(),
             Qt::SolidLine, Qt::RoundCap);
    painter->setPen(pen);
    // See https://github.com/LibrePCB/LibrePCB/issues/1440
    mLineF.isNull() ? painter->drawPoint(mLineF.p1())
                    : painter->drawLine(mLineF);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BGI_NetLine::netLineEdited(const BI_NetLine& obj,
                                BI_NetLine::Event event) noexcept {
  Q_UNUSED(obj);
  switch (event) {
    case BI_NetLine::Event::PositionsChanged:
      updateLine();
      break;
    case BI_NetLine::Event::LayerChanged:
      updateLayer();
      updateVisibility();
      break;
    case BI_NetLine::Event::WidthChanged:
      updateLine();
      break;
    case BI_NetLine::Event::NetSignalNameChanged:
      updateNetSignalName();
      break;
    default:
      qWarning() << "Unhandled switch-case in SGI_NetLine::netLineEdited():"
                 << static_cast<int>(event);
      break;
  }
}

void BGI_NetLine::layerEdited(const GraphicsLayer& layer,
                              GraphicsLayer::Event event) noexcept {
  Q_UNUSED(layer);

  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
      update();
      break;
    case GraphicsLayer::Event::HighlightColorChanged:
      update();
      break;
    case GraphicsLayer::Event::VisibleChanged:
    case GraphicsLayer::Event::EnabledChanged:
      updateVisibility();
      break;
    default:
      break;
  }
}

void BGI_NetLine::updateLine() noexcept {
  prepareGeometryChange();
  mLineF.setP1(mNetLine.getStartPoint().getPosition().toPxQPointF());
  mLineF.setP2(mNetLine.getEndPoint().getPosition().toPxQPointF());
  mBoundingRect = QRectF(mLineF.p1(), mLineF.p2()).normalized();
  mBoundingRect.adjust(
      -mNetLine.getWidth()->toPx() / 2, -mNetLine.getWidth()->toPx() / 2,
      mNetLine.getWidth()->toPx() / 2, mNetLine.getWidth()->toPx() / 2);
  mShape = QPainterPath();
  mShape.moveTo(mNetLine.getStartPoint().getPosition().toPxQPointF());
  mShape.lineTo(mNetLine.getEndPoint().getPosition().toPxQPointF());
  mShape = Toolbox::shapeFromPath(mShape, QPen(Qt::SolidPattern, 0), QBrush(),
                                  positiveToUnsigned(mNetLine.getWidth()));
  update();
}

void BGI_NetLine::updateLayer() noexcept {
  // set Z value
  setZValue(BoardGraphicsScene::getZValueOfCopperLayer(mNetLine.getLayer()));

  // set layer
  if (mLayer) {
    mLayer->onEdited.detach(mOnLayerEditedSlot);
  }
  mLayer = mLayerProvider.getLayer(mNetLine.getLayer());
  if (mLayer) {
    mLayer->onEdited.attach(mOnLayerEditedSlot);
  }
}

void BGI_NetLine::updateNetSignalName() noexcept {
  setToolTip(mNetLine.getNetSegment().getNetNameToDisplay(true));
}

void BGI_NetLine::updateVisibility() noexcept {
  setVisible(mLayer && mLayer->isVisible());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
